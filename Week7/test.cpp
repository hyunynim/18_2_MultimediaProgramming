#include<cstdio>

int main() {
	int * p = new int;
	printf("%p", p);
	delete p;
	printf("\n%p", p);
}