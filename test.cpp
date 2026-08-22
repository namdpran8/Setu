class A { public: virtual ~A() = default; }; class B { public: virtual ~B() = default; }; int main() { A* a = new A(); B* b = dynamic_cast<B*>(a); return 0; }
