#pragma once

#include <variant>
#include <string>
#include <stdexcept>
#include "domain/errors/DomainError.h"

namespace pixcode::application {

template <typename T, typename E = domain::DomainError>
class Result {
public:
    static Result ok(T val) {
        return Result(std::in_place_index<0>, std::move(val));
    }

    static Result error(E err) {
        return Result(std::in_place_index<1>, std::move(err));
    }

    [[nodiscard]] bool isOk() const noexcept {
        return m_storage.index() == 0;
    }

    [[nodiscard]] bool isError() const noexcept {
        return m_storage.index() == 1;
    }

    [[nodiscard]] const T& value() const & {
        if (!isOk()) {
            throw std::logic_error("Attempted to access value from error Result: " + error().message());
        }
        return std::get<0>(m_storage);
    }

    [[nodiscard]] T&& value() && {
        if (!isOk()) {
            throw std::logic_error("Attempted to access value from error Result: " + error().message());
        }
        return std::get<0>(std::move(m_storage));
    }

    [[nodiscard]] const E& error() const noexcept {
        return std::get<1>(m_storage);
    }

private:
    template <std::size_t I, typename... Args>
    explicit Result(std::in_place_index_t<I> tag, Args&&... args)
        : m_storage(tag, std::forward<Args>(args)...) {}

    std::variant<T, E> m_storage;
};

// Void specialization for commands
template <typename E>
class Result<void, E> {
public:
    static Result ok() {
        return Result();
    }

    static Result error(E err) {
        return Result(std::move(err));
    }

    [[nodiscard]] bool isOk() const noexcept {
        return !m_error.has_value();
    }

    [[nodiscard]] bool isError() const noexcept {
        return m_error.has_value();
    }

    [[nodiscard]] const E& error() const {
        if (!isError()) {
            throw std::logic_error("Attempted to access error from ok Result");
        }
        return *m_error;
    }

private:
    Result() = default;
    explicit Result(E err) : m_error(std::move(err)) {}

    std::optional<E> m_error;
};

} // namespace pixcode::application

