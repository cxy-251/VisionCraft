// [=] 看起来是「把用到的都拷贝一份」，但成员变量不是这样：它拷贝的是 this 指针，n 实际是 this->n
struct Widget {
    int n = 1;
    auto makeReader() { return [=] { return n; }; }
};

int main()
{
    auto read = Widget().makeReader();   // 临时的 Widget 在这一行结束时销毁
    return read();                       // 通过悬空的 this 读 n
}
