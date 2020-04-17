//template<typename T>
class RenderHook {
public:

//    std::function<void(T &)> OnPresentFn;

    virtual bool hook() {
        return false;
    }

    virtual bool unhook() {
        return false;
    }
};