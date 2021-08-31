#pragma once

#include <memory>
#include <mutex>
#include <condition_variable>

#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include "wrapper_database.hpp"

namespace py = pybind11;



class PYBIND11_EXPORT wrapper_client final : public boost::intrusive_ref_counter<wrapper_client> {
public:
    wrapper_client();
    wrapper_database_ptr get_or_create(const std::string& name);
    auto database_names() -> py::list;
private:
    wrapper_database_ptr tmp_;
    void d() {
        cv_.notify_all();
        i = 1;
    }
    int i=0;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::unordered_map<std::string, boost::intrusive_ptr<wrapper_database>> storage_;
};