#pragma once
#include <cstddef>
#include <memory>
#include <iosfwd>
#include <stdexcept>

namespace data {

    class IMatrix {
    protected:
        std::size_t rows, columns;
        double* data;


    public:
        IMatrix(std::size_t rows, std::size_t columns);
        virtual ~IMatrix();

        double& operator()(std::size_t row, std::size_t column);

        IMatrix(std::size_t rows, std::size_t columns, double* external_data);

        virtual std::unique_ptr<IMatrix> transpose() = 0;
        virtual std::unique_ptr<IMatrix> operator+(IMatrix& other) = 0;
        virtual std::unique_ptr<IMatrix> operator-(IMatrix& other) = 0;
        virtual std::unique_ptr<IMatrix> operator*(IMatrix& other) = 0;
        virtual std::unique_ptr<IMatrix> operator*(double scalar) = 0;
        virtual std::unique_ptr<IMatrix> operator+(double scalar) = 0;

        std::unique_ptr<IMatrix> operator~();

        double* Data();
        size_t Rows();
        size_t Columns();

        friend std::ostream& operator<<(std::ostream& stream, IMatrix& matrix);
        friend std::istream& operator>>(std::istream& stream, IMatrix& matrix);
    };

}