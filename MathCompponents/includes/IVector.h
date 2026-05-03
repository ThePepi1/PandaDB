#pragma once

#include <cstddef>
#include <memory>
#include <iosfwd>

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
    virtual std::unique_ptr<IVector> operator+(IVector& other) = 0;
    virtual std::unique_ptr<IVector> operator-(IVector& other) = 0;
    virtual double operator*(IVector& other) = 0;
    virtual std::unique_ptr<IVector> operator/(double scalar) = 0;
    virtual std::unique_ptr<IVector> operator*(double scalar) = 0;
    virtual std::unique_ptr<IVector> operator+(double scalar) = 0;
    virtual std::unique_ptr<IVector> operator^(IVector& other) = 0;
    virtual double norm() = 0;
    virtual std::unique_ptr<IVector> normalize() = 0;

    double* Data() const;
    size_t Dimension() const;

    friend std::ostream& operator<<(std::ostream& stream, IVector& matrix);
    friend std::istream& operator>>(std::istream& stream, IVector& matrix);

};




