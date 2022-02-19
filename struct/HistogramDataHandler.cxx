#ifndef HISTOGRAMDATAHANDLER
#define HISTOGRAMDATAHANDLER

#include "../meta/stl.C"
#include "../meta/root.C"
#include "../helpers/root.C"

class UninitializedException : public std::exception {
public:
    UninitializedException() = default;
    ~UninitializedException() noexcept = default;
    const char *what() const noexcept { return "Histogram uninitialized"; }
};

template<typename T>
class UnmanagedHistogramData {
    public:
        UnmanagedHistogramData() = default;
        UnmanagedHistogramData(T *histogram) : fHistogram(histogram) {}
        ~UnmanagedHistogramData()  = default;

        T *getHistogram() const { if(!fHistogram) throw UninitializedException(); return fHistogram; }
        void setHistogram(T *histogram) { fHistogram = histogram; }

    private:
        T *fHistogram = nullptr;
};

template<typename T>
class ManagedHistogramData {
    public:
        ManagedHistogramData() = default;
        ManagedHistogramData(const T *histogram) : fHistogram(static_cast<T *>(histcopy(histogram))) {}
        ~ManagedHistogramData() = default;

        T *getHistogram() const { if(!fHistogram) throw UninitializedException(); return fHistogram.get(); }
        void setHistogram(const T *histogram) { fHistogram = std::shared_ptr<T>(static_cast<T *>(histcopy(histogram))); }

    private:
        std::shared_ptr<T> fHistogram;
};

#endif