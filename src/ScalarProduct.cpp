#include <boost/program_options.hpp>
#include <iostream>
#include <LogHard/Backend.h>
#include <LogHard/FileAppender.h>
#include <LogHard/Logger.h>
#include <LogHard/StdAppender.h>
#include <memory>
#include <sharemind/controller/SystemController.h>
#include <sharemind/controller/SystemControllerConfiguration.h>
#include <sharemind/controller/SystemControllerGlobals.h>
#include <sharemind/DebugOnly.h>
#include <sharemind/GlobalDeleter.h>
#include <sstream>
#include <string>

namespace sm = sharemind;

inline std::shared_ptr<void> newGlobalBuffer(std::size_t const size) {
    auto * const b = size ? ::operator new(size) : nullptr;
    try {
        return std::shared_ptr<void>(b, sm::GlobalDeleter());
    } catch (...) {
        ::operator delete(b);
        throw;
    }
}

inline std::shared_ptr<void> newGlobalBuffer(void const * const data,
                                             std::size_t const size)
{
    auto r(newGlobalBuffer(size));
    std::memcpy(r.get(), data, size);
    return r;
}

int main(int argc, char ** argv) {
    std::string conf;

    try {
        namespace po = boost::program_options;

        po::options_description desc(
            "ScalarProduct\n"
            "Usage: ScalarProduct [OPTION]...\n\n"
            "Options");
        desc.add_options()
            ("conf,c", po::value<std::string>(&conf)->default_value("client.cfg"),
                "Set the configuration file.")
            ("help", "Print this help");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return EXIT_SUCCESS;
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
    std::vector<sm::Int64> a;
    std::vector<sm::Int64> b;

    for (sm::Int64 i = -5; i < 5; ++i) {
        a.push_back(i);
    }
    for (sm::Int64 i = 0; i < 10; ++i) {
        b.push_back(i);
    }

    assert(a.size() == b.size());

    {
        std::ostringstream oss;
        oss << "Vector A: [ ";
        for (const auto val : a) {
            oss << val << ' ';
        }
        oss << ']';
        logger.info() << oss.str();
    }

    {
        std::ostringstream oss;
        oss << "Vector B: [ ";
        for (const auto val : b) {
            oss << val << ' ';
        }
        oss << ']';
        logger.info() << oss.str();
    }

    try {
        sm::SystemControllerConfiguration config(conf);
        sm::SystemControllerGlobals systemControllerGlobals;
        sm::SystemController c(logger, config);

        // Initialize the argument map and set the arguments
        sm::IController::ValueMap arguments;
        arguments["a"] =
                std::make_shared<sm::IController::Value>(
                    "pd_shared3p",
                    "int64",
                    newGlobalBuffer(a.data(), sizeof(sm::Int64) * a.size()),
                    sizeof(sm::Int64) * a.size());
        arguments["b"] =
                std::make_shared<sm::IController::Value>(
                    "pd_shared3p",
                    "int64",
                    newGlobalBuffer(b.data(), sizeof(sm::Int64) * b.size()),
                    sizeof(sm::Int64) * b.size());

        // Run code
        logger.info() << "Sending secret shared arguments and running SecreC bytecode on the servers";
        sm::IController::ValueMap results = c.runCode("scalar_product.sb", arguments);

        // Print the result
        sm::IController::ValueMap::const_iterator it = results.find("c");
    if (it == results.end()) {
            logger.error() << "Missing 'c' result value.";
            return EXIT_FAILURE;
	}

	try {
            sm::Int64 c = it->second->getValue<sm::Int64>();
            logger.info() << "The computed scalar product is: " << c;
        } catch (const sm::IController::Value::ParseException & e) {
            logger.error() << "Failed to cast 'c' to appropriate type: " <<
                e.what();
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    } catch (const sm::IController::WorkerException & e) {
        logger.error() << "Multiple exceptions caught:";
        for (size_t i = 0u; i < e.numWorkers(); i++) {
            std::exception_ptr ep(e.nested_ptrs()[i]);
            while (ep) {
                try {
                    std::rethrow_exception(e.nested_ptrs()[i]);
                } catch (const std::exception & e) {
                    logger.error() << ' ' << i << ": " << e.what();
                    try {
                        std::rethrow_if_nested(e);
                        ep = nullptr;
                    } catch (...) {
                        ep = std::current_exception();
                    }
                } catch (const std::nested_exception & e) {
                    logger.error() << ' ' << i << ": <unknown nested>";
                    ep = e.nested_ptr();
                } catch (...) {
                    logger.error() << ' ' << i << ": <unknown>";
                    ep = nullptr;
                }
            }
        }
    } catch (const std::exception & e) {
        logger.error() << "Caught exception: " << e.what();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
