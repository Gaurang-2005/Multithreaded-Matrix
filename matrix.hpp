#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <thread>

class task_queue {

    int** q;
    size_t next;
    size_t s;
    size_t elemsize;

public:

    task_queue(size_t a, size_t b) : next{0}, s{a * b}, elemsize{s} {

        q = new int*[s];

        for (size_t i = 0; i < s; i++) {
            q[i] = new int[2];
        }

        for (size_t i = 0; i < a; i++) {
            for (size_t j = 0; j < b; j++) {
                q[i * a + j][0] = i;
                q[i * a + j][1] = j;
            }
        }
    }
    int* pop() {
        if (next >= s) return nullptr;
        elemsize--;
        return q[next++];
    }

    size_t elesize() const {return elemsize;}

};

template<typename T>
class matrix {
    size_t row;
    size_t col;
    T* mat;

public:
    matrix() : row{0}, col{0}, mat{nullptr} {}

    matrix(size_t a, size_t b): row{a}, col{b} {
        mat = new T[row * col]{0};
    }

    //copy constructor
    matrix(const matrix<T>& temp) : row{temp.row}, col{temp.col} {
        mat = new T[row * col];
        for (size_t i = 0; i < row * col; i++) {
            mat[i] = temp.mat[i];
        }
    }

    //move constructor
    matrix(matrix<T>&& temp) noexcept : row{temp.row}, col{temp.col} {
        temp.row = 0;
        temp.col = 0;

        mat = temp.mat;
        temp.mat = nullptr;
    }

    //move assignment
    matrix<T>& operator=(matrix<T>&& temp) noexcept {
        if (&temp == this) return *this;
        delete[] mat;
        mat = temp.mat;
        row = temp.row;
        col = temp.col;
        temp.mat = nullptr;
        temp.row = 0;
        temp.col = 0;

        return *this;
    }

    //copy assignment
    matrix<T>& operator=(const matrix<T>& temp) {
        if (&temp == this) return *this;
        T* temp2 = new T[temp.row * temp.col];
        for (size_t i = 0; i < temp.row * temp.col; i++) {
            temp2[i] = temp.mat[i];
        }
        delete[] mat;
        mat = temp2;
        row = temp.row;
        col = temp.col;

        return *this;
    }

    //number of rows getter
    size_t rows() const {
        return row;
    }

    //number of columns getter
    size_t cols() const {
        return col;
    }

    T& operator()(size_t a, size_t b) {
        assert(a < row && b < col);
        return mat[a * (col) + b];
    }

    const T& operator()(size_t a, size_t b) const {
        assert(a < row && b < col);
        return mat[a * (col) + b];
    }

    void MultiplicationLoop(matrix<T>& result, const matrix<T>& second, size_t blockSize, size_t blockX1, size_t blockY1, size_t blockX2, size_t blockY2) const {
        
        size_t maxX = std::min(this->col, blockSize*(blockX2 + 1)) - blockSize * blockX2;
        size_t maxY = std::min(this->row, blockSize * (blockY1 + 1)) - blockSize * blockY1;
        size_t common = std::min(this->col, blockSize * (blockX1 + 1)) - blockSize * blockX1;

        size_t ir1 = blockY1 * blockSize;
        size_t jr1 = blockX1 * blockSize;
        size_t ir2 = blockY2 * blockSize;
        size_t jr2 = blockX2 * blockSize;        
        
        for (size_t i = 0; i < maxY; i++) {
            for (size_t j = 0; j < maxX; j++) {
                for (size_t k = 0; k < common; k++) {
                    result(ir1 + i, jr2 + j) += (*this)(ir1 + i, jr1 + k) * second(ir2 + k, jr2 + j);
                }
            }
        }
    }

    void blockRange(matrix<T>& result, const matrix<T>& second, size_t blockSize, task_queue& q, size_t blockX2) const {
        size_t x2 = 0, y2 = 0;
        while (q.elesize()) {
            auto coord = q.pop();
            if (!coord) return;
            for (size_t i = 0; i < blockX2; i++) {
                MultiplicationLoop(result, second, blockSize, coord[0], coord[1], i, coord[0]);
            }
        }
    }

    //With Blocking:
    matrix<T> operator*(const matrix<T>& second) const {
        assert(col == second.row);
        int hardThreads = std::thread::hardware_concurrency();
        hardThreads = (hardThreads > 0) ? hardThreads : 1;

        size_t blockSize = 32;

        size_t blockX1 = (col % blockSize == 0)? col / blockSize : col / blockSize + 1;
        size_t blockY1 = (row % blockSize == 0)? row / blockSize : row / blockSize + 1;
        size_t blockX2 = (second.col % blockSize == 0)? second.col / blockSize : second.col / blockSize + 1;
        // size_t blockY2 = (second.row % blockSize == 0)? second.row / blockSize : second.row / blockSize + 1;

        unsigned int blocks = (blockY1 * blockX2);

        unsigned int maxThreads = std::min(hardThreads, int(blocks));
        task_queue q(blockY1, blockX1);

        matrix<T> result(row, second.col);
        std::unique_ptr<std::thread[]> threadArr (new std::thread[maxThreads]);

        for (unsigned int i = 0; i < maxThreads; i++) {
            threadArr[i] = std::thread(&matrix<T>::blockRange, this, std::ref(result), std::cref(second), blockSize, std::ref(q), blockX2); 
        }
        for (unsigned int i = 0; i < maxThreads; i++) threadArr[i].join();

        return result;
    }
    
    matrix<T>& operator*=(const matrix<T>& second) {
        assert(col == second.row);
        int hardThreads = std::thread::hardware_concurrency();
        hardThreads = (hardThreads > 0) ? hardThreads : 1;
        unsigned int maxThreads = std::min(hardThreads, int(row));

        matrix<T> result(row, second.col);
        size_t rowsPerThread = result.row / maxThreads;
        std::unique_ptr<std::thread[]> threadArr (new std::thread[maxThreads]);

        for (unsigned int i = 0; i < maxThreads; i++) {
            if (i == maxThreads - 1) {
                threadArr[i] = std::thread(&matrix<T>::MultiplicationLoop, this, std::ref(result), std::cref(second), rowsPerThread * i, result.row); 
                break;
            }
            threadArr[i] = std::thread(&matrix<T>::MultiplicationLoop, this, std::ref(result), std::cref(second), rowsPerThread * i, rowsPerThread * (i + 1)); 
        }
        for (unsigned int i = 0; i < maxThreads; i++) threadArr[i].join();
        *this = result;
        return *this;
    }
    matrix<T> operator*(T num) const {
        matrix<T> result(row, col);

        for (size_t i = 0; i < row; i++) {
            for (size_t j = 0; j < col; j++) {
                result(i, j) = (*this)(i, j) * num;
            }
        }

        return result;
    }

    matrix<T>& operator*=(T num) {

        for (size_t i = 0; i < row; i++) {
            for (size_t j = 0; j < col; j++) {
                (*this)(i, j) *= num;
            }
        }

        return *this;
    }
    
    friend matrix<T> operator*(const T num, const matrix<T>& temp) {
        matrix<T> result(temp.row, temp.col);

        for (size_t i = 0; i < temp.row; i++) {
            for (size_t j = 0; j < temp.col; j++) {
                result(i, j) = temp(i, j) * num;
            }
        }

        return result;
    }

    matrix<T> transpose() const {
        matrix<T> result(col, row);

        for (size_t i = 0; i < row; i++) {
            for (size_t j = 0; j < col; j++) {
                result(j, i) = (*this)(i, j);
            }
        }

        return result;
    }

    ~matrix() noexcept {
        delete[] mat;
    }
};