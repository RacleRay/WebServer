#pragma once

class Noncopyable {
protected:
    Noncopyable() = default;
    ~Noncopyable() = default;

public:
    Noncopyable(const Noncopyable&) = delete;
    const Noncopyable& operator=(const Noncopyable&) = delete;

// private:
//     Noncopyable(const Noncopyable&);
//     const Noncopyable& operator=(const Noncopyable&);
};