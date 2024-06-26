/**
 * @file    fixed_precision.h
 * Wraps a floating point number for associative +/- operations (fixed precision)
 *         
 */
// Copyright (c) 2013-2015 OpenM++
// This code is licensed under the MIT license (see LICENSE.txt for details)

#pragma once

template<typename T>
class fixed_precision
{
public:
    // expose the wrapped type as .type
    typedef T type;

    // default ctor
    fixed_precision()
        : value(0)
    {}

    // converting ctor to wrapped type T
    fixed_precision(T value)
    {
        this->set(value);
    }

    // operator: user-defined conversion to T
    operator T() const
    {
        return value;
    }

    // operator: assignment by sum
    fixed_precision& operator+=( T modify_value )
    {
        T new_value = value + modify_value;
        this->set( new_value );
        return *this;
    }

    // operator: assignment by difference
    fixed_precision& operator-=( T modify_value )
    {
        T new_value = value - modify_value;
        this->set( new_value );
        return *this;
    }

    // operator: assignment by product
    fixed_precision& operator*=( T modify_value )
    {
        T new_value = value * modify_value;
        this->set( new_value );
        return *this;
    }

    // operator: assignment by quotient
    fixed_precision& operator/=( T modify_value )
    {
        T new_value = value / modify_value;
        this->set( new_value );
        return *this;
    }

    // operator: prefix increment
    fixed_precision& operator++()
    {
        T new_value = value + 1;
        this->set( new_value );
        return *this;
    }

    // operator: prefix decrement
    fixed_precision& operator--()
    {
        T new_value = value - 1;
        this->set( new_value );
        return *this;
    }

    // operator: postfix increment
    T operator++(int)
    {
        T new_value = 1 + value;
        this->set( new_value );
        return new_value;
    }

    // operator: postfix decrement
    T operator--(int)
    {
        T new_value = value - 1;
        this->set( new_value );
        return new_value;
    }

    /**
     * Specify the maximum value to be used (global).
     * 
     * This value is used to calculate how many excess binary digits of precision are to be dropped
     * in all subsequent assignments to objects of the type fixed_precision.  The special
     * value 0 indicates that objects will store values with no modification.
     *
     * @param max_val The maximum value.
     */
    static void set_max(double max_val)
    {
        if (max_val < 1) {
            // disable
            addend = 0;
            return;
        }
        // binary digits required for integer part (less 1)
        int binary_digits = (int) (log(max_val) / log(2.0));
        // size of addend is 2 raised to that power
        addend = 1;
        for (int i = 0; i < binary_digits; ++i) {
            addend *= 2;
        }
    }

private:
    void set(T val)
    {
        value = val;
        if (addend == 0) {
            return;
        }
        else {
            if (value > 0) {
                if (value < addend) {
                    add_addend(); // adding causes excess digits of precision to 'fall off' the right
                    subtract_addend(); // subtracting restores the original value less the dropped digits
                }
            }
            else {
                if (value > -addend) {
                    subtract_addend();
                    add_addend();
                }
            }
        }
    }

    void add_addend()
    {
        value = value + addend;
    }

    void subtract_addend()
    {
        value = value - addend;
    }

    /**
     * Storage for the wrapped value.
     */
    T value;


    /**
     * The specified maximum value, rounded up to the next power of 2
     */
    static int addend;
};

/**
* Specified maximum value, rounded up to the next power of 2 (definition)
*/
template<typename T>
int fixed_precision<T>::addend = 0;

// Enable fixed_precision to participate in type resolution
// e.g. in min/max/clamp templates, by specializing std::common_type

#if false // in testing
namespace std {

    template<typename T>
    struct common_type<fixed_precision<T>>
    {
        using type = T;
    };

    template<typename T>
    struct common_type<fixed_precision<T>, fixed_precision<T>>
    {
        using type = T;
    };

    template<typename T, typename U>
    struct common_type<fixed_precision<T>, U>
    {
        using type = typename common_type<T, U>::type;
    };

    // opposite order to above
    template<typename T, typename U>
    struct common_type<U, fixed_precision<T>>
    {
        using type = typename common_type<T, U>::type;
    };

}
#endif
