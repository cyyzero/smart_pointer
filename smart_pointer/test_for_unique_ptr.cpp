#include "unique_ptr.h"
#include <iostream>
#include <string>
#include <cassert>
#include <memory>
// std::unique_ptr<>
// The example above is from cppreference

// It is tests for unique_ptr for single object
struct Fooo {
    Fooo(int _val) : val(_val) { std::cout << "Fooo...\n"; }
    ~Fooo() { std::cout << "~Fooo...\n"; }
    int val;
};


struct Foo { // 要管理的对象
    Foo() { std::cout << "Foo ctor\n"; }
    Foo(const Foo&) { std::cout << "Foo copy ctor\n"; }
    Foo(Foo&&) { std::cout << "Foo move ctor\n"; }
    ~Foo() { std::cout << "~Foo dtor\n"; }
};
 
struct D { // 删除器
    D() {};
    D(const D&) { std::cout << "D copy ctor\n"; }
    D(D&) { std::cout << "D non-const copy ctor\n";}
    D(D&&) { std::cout << "D move ctor \n"; }
    void operator()(Foo* p) const {
        std::cout << "D is deleting a Foo\n";
        delete p;
    };
};

struct Vec3
{
    int x, y, z;
    Vec3() : x(0), y(0), z(0) { }
    Vec3(int x, int y, int z) :x(x), y(y), z(z) { }
    friend std::ostream& operator<<(std::ostream& os, Vec3& v) {
        return os << '{' << "x:" << v.x << " y:" << v.y << " z:" << v.z  << '}';
    }
};

int main()
{
    // Tests for constructors
    std::cout << "Example constructor(1)...\n";
    sm_ptr::unique_ptr<Foo> up1;  // up1 为空
    sm_ptr::unique_ptr<Foo> up1b(nullptr);  // up1b 为空
 
    std::cout << "Example constructor(2)...\n";
    {
        sm_ptr::unique_ptr<Foo> up2(new Foo); // up2 现在占有 Foo
    } // Foo 被删除
 
    std::cout << "Example constructor(3)...\n";
    D d;
    {  // 删除器类型不是引用
       sm_ptr::unique_ptr<Foo, D> up3(new Foo, d); // 复制删除器
    }
    {  // 删除器类型是引用 
       sm_ptr::unique_ptr<Foo, D&> up3b(new Foo, d); // up3b 保有到 d 的引用
    }
 
    std::cout << "Example constructor(4)...\n";
    {  // 删除器不是引用
       sm_ptr::unique_ptr<Foo, D> up4(new Foo, D()); // 移动删除器
    }
 
    std::cout << "Example constructor(5)...\n";
    {
       sm_ptr::unique_ptr<Foo> up5a(new Foo);
       sm_ptr::unique_ptr<Foo> up5b(std::move(up5a)); // 所有权转移
    }
 
    std::cout << "Example constructor(6)...\n";
    {
        sm_ptr::unique_ptr<Foo, D> up6a(new Foo, d); // 复制 D
        sm_ptr::unique_ptr<Foo, D> up6b(std::move(up6a)); // 移动 D
 
        sm_ptr::unique_ptr<Foo, D&> up6c(new Foo, d); // D 是引用
        sm_ptr::unique_ptr<Foo, D> up6d(std::move(up6c)); // 复制 D
    }


    // Test for destructors
    {
        auto deleter = [](int* ptr){
            std::cout << "[deleter called]\n";
            delete ptr;
        };
    
        sm_ptr::unique_ptr<int,decltype(deleter)> uniq(new int, deleter);
        std::cout << (uniq ? "not empty\n" : "empty\n");
        uniq.reset();
        std::cout << (uniq ? "not empty\n" : "empty\n");
    }


    // Tests for member function release()
    {
        std::cout << "Creating new Foo...\n";
        sm_ptr::unique_ptr<Foo> up(new Foo());
    
        std::cout << "About to release Foo...\n";
        Foo* fp = up.release();
    
        assert (up.get() == nullptr);
        std::cout << "Foo is no longer owned by unique_ptr...\n";
    
        delete fp;
    }

    // Tests for reset()
    {
        std::cout << "Creating new Foo...\n";
        sm_ptr::unique_ptr<Foo, D> up(new Foo(), D());  // up 占有 Foo 指针（删除器 D ）
    
        std::cout << "Replace owned Foo with a new Foo...\n";
        up.reset(new Foo());  // 调用旧者的删除器
    
        std::cout << "Release and delete the owned Foo...\n";
        up.reset(nullptr);
    }

    // Tests for swap()
    {
        sm_ptr::unique_ptr<Fooo> up1(new Fooo(1));
        sm_ptr::unique_ptr<Fooo> up2(new Fooo(2));
    
        up1.swap(up2);
    
        std::cout << "up1->val:" << up1->val << std::endl;
        std::cout << "up2->val:" << up2->val << std::endl;
    }

    // Tests for get()
    {
        sm_ptr::unique_ptr<std::string> s_p(new std::string("Hello, world!"));
        auto s = s_p.get();
        std::cout << *s << std::endl;
    }
    {
        sm_ptr::unique_ptr<int[]> ptr(new int[10]);
        for (int i = 0; i < 10; ++i)
            ptr[i] = i;
    }
    {
        sm_ptr::unique_ptr<int> p1(new int(42));
        sm_ptr::unique_ptr<int> p2(new int(42));
    
        std::cout << "p1 == p1: " << (p1 == p1) << '\n';
    
        // p1 and p2 point to different memory locations, so p1 != p2
        std::cout << "p1 == p2: " << (p1 == p2) << '\n';
    }

    {
        // 使用默认构造函数。
        sm_ptr::unique_ptr<Vec3> v1 = sm_ptr::make_unique<Vec3>();
        // 使用匹配这些参数的构造函数
        sm_ptr::unique_ptr<Vec3> v2 = sm_ptr::make_unique<Vec3>(0, 1, 2);
        // 创建指向 5 个元素数组的 unique_ptr 
        sm_ptr::unique_ptr<Vec3[]> v3 = sm_ptr::make_unique<Vec3[]>(5);
    
        std::cout << "make_unique<Vec3>():      " << *v1 << '\n'
                  << "make_unique<Vec3>(0,1,2): " << *v2 << '\n'
                  << "make_unique<Vec3[]>(5):   " << '\n';
        for (int i = 0; i < 5; i++) {
            std::cout << "     " << v3[i] << '\n';
        }
    }
}
