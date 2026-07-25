#pragma once

#include <cstddef>
#include <memory>
#include <iosfwd>
namespace data {
    class IVector {
    private:
        bool owner = true;
    protected:
        std::size_t dimension;
        double* data;
    public:
        IVector(std::size_t dimension);
        IVector(std::size_t dimension, double* external_data);
        IVector(std::size_t dimension, double data);
        virtual ~IVector();

        double* operator[](std::size_t index) const;
        double& operator()(std::size_t index) const;
        virtual std::unique_ptr<IVector> operator+(const IVector& other)const = 0;
        virtual std::unique_ptr<IVector> operator-(const IVector& other) const = 0;
        virtual double operator*(const IVector& other) const = 0;
        virtual std::unique_ptr<IVector> operator/(double scalar)const = 0;
        virtual std::unique_ptr<IVector> operator*(double scalar) const = 0;
        virtual std::unique_ptr<IVector> operator+(double scalar)const = 0;
        virtual std::unique_ptr<IVector> operator^(const IVector& other)const = 0;
        virtual double norm() const = 0;
        virtual std::unique_ptr<IVector> normalize() const = 0;

        double* Data() const;
        size_t Dimension() const;
        bool Owner() const;
        void Disown();
        friend std::ostream& operator<<(std::ostream& stream, IVector& matrix);
        friend std::istream& operator>>(std::istream& stream, IVector& matrix);

    };
}




