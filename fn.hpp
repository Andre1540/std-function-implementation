#ifndef FN_HPP
#define FN_HPP

#include <memory>
#include <cstdarg>
#include <typeinfo>
#include <type_traits>

namespace fn {

    struct bad_function_call : std::exception { };

    template<class>
    class function;

    template <class R, class ... ArgumentTypes>
    void swap(function < R (ArgumentTypes ...) > & a, function < R (ArgumentTypes ...)> & b)
    {
        a.swap(b);
    }


    template <class R, class ... ArgumentTypes>
    class function <R (ArgumentTypes ...) >
    {

        using freeFuncPtr_t = R (*)(ArgumentTypes...);

        function & clone (const function & a)
        {
            if (a.freeFuncPtr_)
            {
                freeFuncPtr_ = a.freeFuncPtr_;
                funcPtr_ = nullptr;
            }
            else if (a.funcPtr_)
            {
                freeFuncPtr_ = nullptr;
                funcPtr_ = (a.funcPtr_).get()->clone();
            }
            else
            {
                freeFuncPtr_ = nullptr;
                funcPtr_ = nullptr;
            }
            return *this;
        }

    public:
        function ()
                : freeFuncPtr_(nullptr)
                , funcPtr_(nullptr)
        {}

        function (std::nullptr_t)
                : freeFuncPtr_(nullptr)
                , funcPtr_(nullptr)
        {}

        function (function && a)
                : freeFuncPtr_(std::move(a.freeFuncPtr_))
                , funcPtr_(std::move(a.funcPtr_))
        {}

        function (const function & a)
        {
            clone(a);
        }



        template <bool> struct tag_type {};

        template <typename Functor>
        void function_impl (Functor const & f, tag_type<false>)     // for other functions
        {
            freeFuncPtr_ = nullptr;
            funcPtr_.reset(new holder<Functor>(f));
        }

        template<class Functor>
        void function_impl (Functor const & f, tag_type<true>)      // for free functions
        {
            // Если сигнатура функции f не совпадает с сигнатурой <R (Arg)>
            // То запустить function_impl (Functor const & f, tag_type<false>)
            bool b = (typeid(f) == typeid(R (*)(ArgumentTypes...)));
            if (!b)
            {
                function_impl (f, tag_type<false>());
                return;
            }

            freeFuncPtr_ = ((freeFuncPtr_t)f);
            funcPtr_ = nullptr;
        }



        template<class Functor>
        function (Functor f)
        {
            using cleanType1 = typename std::remove_reference<Functor>::type;
            using cleanType2 = typename std::remove_pointer<cleanType1>::type;

            function_impl<Functor>(f, tag_type<std::is_function<cleanType2>::value>());
        }

        function & operator = (function && a)
        {
            freeFuncPtr_ = std::move(a.freeFuncPtr_);
            funcPtr_ = std::move(a.funcPtr_);
            return *this;
        }

        function & operator = (const function & a)
        {
            return clone(a);
        }

        R operator() (ArgumentTypes &&...args) const
        {
            if (!(*this))
            {
                throw fn::bad_function_call();
            }

            if (funcPtr_)
            {
                return funcPtr_->call(std::forward<ArgumentTypes>(args)...);
            }
            return (freeFuncPtr_)(std::forward<ArgumentTypes>(args)...);
        }

        R operator() (ArgumentTypes && ... args)
        {
            if (!(*this))
            {
                throw fn::bad_function_call();
            }
            if (funcPtr_)
            {
                return funcPtr_->call(std::forward<ArgumentTypes>(args)...);
            }
            return (freeFuncPtr_)(std::forward<ArgumentTypes>(args)...);
        }

        explicit operator bool () const
        {
            return (funcPtr_ != nullptr || freeFuncPtr_ != nullptr);
        }

        explicit operator bool ()
        {
            return (funcPtr_ != nullptr || freeFuncPtr_ != nullptr);
        }

        void swap(function & b)
        {
            std::swap(funcPtr_, b.funcPtr_);
            std::swap(freeFuncPtr_, b.freeFuncPtr_);
        }

    private:
        struct base_holder;
        using base_holder_ptr = std::shared_ptr<base_holder>;

        // ==============  Идиома Type Erasure ==================
        // ========== Базовый класс функции  ====================
        struct base_holder
        {
            base_holder () {}
            virtual ~base_holder () {}
            virtual R call(ArgumentTypes ... args) = 0;
            virtual base_holder_ptr clone() = 0;

            // =========== Запрещенные методы =====================
            base_holder(const base_holder &) = delete;
            void operator=(const base_holder &) = delete;
        };

        template <class Functor>
        struct holder : base_holder
        {
            holder(Functor f)
                    : functor_(f)  {}

            virtual R call (ArgumentTypes... args) override
            {
                return functor_(std::forward<ArgumentTypes>(args) ...);
            }
            virtual base_holder_ptr clone() override
            {
                return base_holder_ptr(new holder(functor_));
            }
            Functor functor_;
        };

    private:
        freeFuncPtr_t   freeFuncPtr_;
        base_holder_ptr funcPtr_;
    };
}

#endif // FN_HPP


