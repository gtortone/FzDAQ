#ifndef FZ_CBUFFER_H
#define FZ_CBUFFER_H

#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/circular_buffer.hpp>

// Thread safe circular buffer 
template <typename T>
class FzCbuffer: private boost::noncopyable
{
public:
    typedef boost::mutex::scoped_lock lock;

    FzCbuffer() {}

    FzCbuffer(int n) {cb.set_capacity(n);}

    void send (T imdata) {
        lock lk(monitor);
        while(cb.full())
            buffer_not_full.wait(lk);
        cb.push_back(imdata);
        buffer_not_empty.notify_one();
    }
   
    T receive() {
        lock lk(monitor);

        while (cb.empty())
            buffer_not_empty.wait(lk);

        T imdata = cb.front();

        cb.pop_front();
        buffer_not_full.notify_one();
        return imdata;
    }
   
    void clear() {
        lock lk(monitor);
        cb.clear();
    }

    int size() {
        lock lk(monitor);
        return cb.size();
    }

    int capacity() {
        lock lk(monitor);
        return cb.capacity();
    }

    void set_capacity(int capacity) {
        lock lk(monitor);
        cb.set_capacity(capacity);
    }

private:
    boost::condition buffer_not_full;
    boost::condition buffer_not_empty;
    boost::mutex monitor;
    boost::circular_buffer<T> cb;

};

#endif
