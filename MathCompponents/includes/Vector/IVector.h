#pragma once

#include <cstddef>
#include <memory>
#include <iosfwd>
#include <../../File/includes/fileHendler.h>
namespace data {
    class IVector {
    private:
        bool owner = true;
    protected:
        std::size_t dimension;
        double* data;
    public:
        IVector() ;
        IVector(std::size_t dimension, double* external_data, bool owner);
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
        virtual IVector& operator+= (const IVector& other) = 0;
        virtual IVector& operator-= (const IVector& other) = 0;
        virtual IVector& operator*= (double scalar) = 0;
        virtual IVector& operator/= (double scalar) = 0;
        virtual IVector& operator+= (double scalar) = 0;
        virtual IVector& operator-= (double scalar) = 0;
        virtual bool operator==(const IVector& other) = 0;
        virtual void operator=(const IVector& other) = 0;
        bool operator!=(const IVector& other) ;
        double* Data() const;
        size_t Dimension() const;
        bool Owner() const;
        void Disown();
        friend std::ostream& operator<<(std::ostream& stream, IVector& matrix);
        friend std::istream& operator>>(std::istream& stream, IVector& matrix);
        friend file::fileReaderWriter& operator<<(file::fileReaderWriter& stream, const IVector& matrix);
        friend file::fileReader& operator>>(file::fileReader& stream, IVector& matrix);
        friend file::fileReaderWriter& operator>>(file::fileReaderWriter& stream,  IVector& matrix);
        static size_t getSizeInBytes(size_t dimension);
    };
}




