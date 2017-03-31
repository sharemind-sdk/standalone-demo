import stdlib;
import shared3p;

domain pd_shared3p shared3p;


void main() {
	pd_shared3p uint64 [[1]] a = argument("a");
	pd_shared3p uint64 [[1]] b = argument("b");

	pd_shared3p uint64 c = sum(a * b);
	publish("c", c);
}
