/*
 * Copyright (C) 2018 Cybernetica
 *
 * Research/Commercial License Usage
 * Licensees holding a valid Research License or Commercial License
 * for the Software may use this file according to the written
 * agreement between you and Cybernetica.
 *
 * GNU General Public License Usage
 * Alternatively, this file may be used under the terms of the GNU
 * General Public License version 3.0 as published by the Free Software
 * Foundation and appearing in the file LICENSE.GPL included in the
 * packaging of this file.  Please review the following information to
 * ensure the GNU General Public License version 3.0 requirements will be
 * met: http://www.gnu.org/copyleft/gpl-3.0.html.
 *
 * For further information, please contact us at sharemind@cyber.ee.
 */

#include <boost/program_options.hpp>
#include <cstdint>
#include <iostream>
#include <LogHard/Backend.h>
#include <LogHard/FileAppender.h>
#include <LogHard/Logger.h>
#include <LogHard/StdAppender.h>
#include <memory>
#include <sharemind/controller/SystemController.h>
#include <sharemind/controller/SystemControllerConfiguration.h>
#include <sharemind/controller/SystemControllerGlobals.h>
#include <sstream>
#include <string>

namespace sm = sharemind;

int main(int argc, char ** argv) {
    std::unique_ptr<sm::SystemControllerConfiguration> config;

    try {
        namespace po = boost::program_options;

        po::options_description desc(
            "ScalarProduct\n"
            "Usage: ScalarProduct [OPTION]...\n\n"
            "Options");
        desc.add_options()
            ("conf,c", po::value<std::string>(),
                "Set the configuration file.")
            ("help", "Print this help");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return EXIT_SUCCESS;
        }
        if (vm.count("conf")) {
            config = std::make_unique<sm::SystemControllerConfiguration>(
                         vm["conf"].as<std::string>());
        } else {
            config = std::make_unique<sm::SystemControllerConfiguration>();
        }
    } catch(std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    auto logBackend(std::make_shared<LogHard::Backend>());
    logBackend->addAppender(std::make_shared<LogHard::StdAppender>());
    logBackend->addAppender(
                std::make_shared<LogHard::FileAppender>(
                    "ScalarProduct.log",
                    LogHard::FileAppender::OVERWRITE));
    const LogHard::Logger logger(logBackend);

    logger.info() << "This is a stand alone Sharemind application demo";
    logger.info() << "It privately computes the scalar product of the following two vectors";

    // Generate some user for input
    auto a = std::make_shared<std::vector<std::int64_t>>();
    auto b = std::make_shared<std::vector<std::int64_t>>();

    for (std::int64_t i = -5; i < 5; ++i) {
        a->push_back(i);
    }
    for (std::int64_t i = 0; i < 10; ++i) {
        b->push_back(i);
    }

    assert(a->size() == b->size());

    {
        std::ostringstream oss;
        oss << "Vector A: [ ";
        for (const auto val : *a) {
            oss << val << ' ';
        }
        oss << ']';
        logger.info() << oss.str();
    }

    {
        std::ostringstream oss;
        oss << "Vector B: [ ";
        for (const auto val : *b) {
            oss << val << ' ';
        }
        oss << ']';
        logger.info() << oss.str();
    }

    try {
        sm::SystemControllerGlobals systemControllerGlobals;
        sm::SystemController c(logger, *config);

        // Initialize the argument map and set the arguments
        sm::SystemController::ValueMap arguments;
        arguments["a"] =
                std::make_shared<sm::SystemController::Value>(
                    "pd_shared3p",
                    "int64",
                    std::shared_ptr<std::int64_t>(a, a->data()),
                    sizeof(std::int64_t) * a->size());
        arguments["b"] =
                std::make_shared<sm::SystemController::Value>(
                    "pd_shared3p",
                    "int64",
                    std::shared_ptr<std::int64_t>(b, b->data()),
                    sizeof(std::int64_t) * b->size());

        // Run code
        logger.info() << "Sending secret shared arguments and running SecreC bytecode on the servers";
        sm::SystemController::ValueMap results = c.runCode("scalar_product.sb", arguments);

        // Print the result
        sm::SystemController::ValueMap::const_iterator it = results.find("c");
    if (it == results.end()) {
            logger.error() << "Missing 'c' result value.";
            return EXIT_FAILURE;
	}

	try {
            auto c = it->second->getValue<std::int64_t>();
            logger.info() << "The computed scalar product is: " << c;
        } catch (const sm::SystemController::Value::ParseException & e) {
            logger.error() << "Failed to cast 'c' to appropriate type: " <<
                e.what();
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    } catch (const sm::SystemController::WorkerException & e) {
        logger.fatal() << "Multiple exceptions caught:";
        for (size_t i = 0u; i < e.numWorkers(); i++) {
            if (std::exception_ptr ep = e.nested_ptrs()[i]) {
                logger.fatal() << "  Exception from server " << i << ':';
                try {
                    std::rethrow_exception(std::move(ep));
                } catch (...) {
                    logger.printCurrentException<LogHard::Priority::Fatal>(
                            LogHard::Logger::StandardExceptionFormatter(4u));
                }
            }
        }
    } catch (const std::exception & e) {
        logger.error() << "Caught exception: " << e.what();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
