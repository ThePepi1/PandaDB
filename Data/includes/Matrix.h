#pragma once
#include <cstddef>
#include <iosfwd>


namespace data {

    class IMatrix {
    protected:
        std::size_t rows, columns;
        double * data;

    public:
        IMatrix(std::size_t rows, std::size_t columns);

        virtual ~IMatrix();

        double & operator()(std::size_t row, std::size_t column);
        virtual IMatrix* transpose() = 0;
        virtual IMatrix* operator+( IMatrix & other) = 0;
        virtual IMatrix* operator-( IMatrix & other)= 0;
        virtual IMatrix* operator*( IMatrix & other) = 0;
        virtual IMatrix* operator*(const double scalar) = 0; // No need for / it is 1/scalar
        virtual IMatrix* operator+(const double scalar) = 0; // no need for - it is -scalar :D
        double* Data();
        size_t Rows();
        size_t Columns();
        friend std::istream & operator>>(std::istream & stream, IMatrix & matrix);
        friend std::ostream & operator<<(std::ostream & stream, IMatrix & matrix);
        IMatrix* operator~();
    };
}
