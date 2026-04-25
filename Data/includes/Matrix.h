#pragma once
#include <cstddef>
#include <memory>
#include <iosfwd>
#include <stdexcept>

namespace data {

    class IMatrix {
    protected:
        std::size_t rows, columns;
        std::size_t capacity;
        double* data;

        void ensure_capacity(std::size_t new_cols);

    public:
        IMatrix(std::size_t rows, std::size_t columns);
        virtual ~IMatrix();

        double& operator()(std::size_t row, std::size_t column);

        // dynamic column push
        void push_column(const double* col_values);

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