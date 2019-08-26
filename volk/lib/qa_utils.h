#ifndef VOLK_QA_UTILS_H
#define VOLK_QA_UTILS_H

#include <stdbool.h>            // for bool, false
#include <volk/volk.h>          // for volk_func_desc_t
#include <cstdlib>              // for NULL
#include <map>                  // for map
#include <string>               // for string, basic_string
#include <vector>               // for vector

#include "volk/volk_complex.h"  // for lv_32fc_t

/************************************************
 * VOLK QA type definitions                     *
 ************************************************/
struct volk_type_t {
    bool is_float;
    bool is_scalar;
    bool is_signed;
    bool is_complex;
    int size;
    std::string str;
};

class volk_test_time_t {
    public:
        std::string name;
        double time;
        std::string units;
        bool pass;
};

class volk_test_results_t {
    public:
        std::string name;
        std::string config_name;
        unsigned int vlen;
        unsigned int iter;
        std::map<std::string, volk_test_time_t> results;
        std::string best_arch_a;
        std::string best_arch_u;
};

class volk_test_params_t {
    private:
        float _tol;
        lv_32fc_t _scalar;
        unsigned int _vlen;
        unsigned int _iter;
        bool _benchmark_mode;
        bool _absolute_mode;
        std::string _kernel_regex;
    public:
        // ctor
        volk_test_params_t(float tol, lv_32fc_t scalar, unsigned int vlen, unsigned int iter,
                           bool benchmark_mode, std::string kernel_regex) :
            _tol(tol), _scalar(scalar), _vlen(vlen), _iter(iter),
            _benchmark_mode(benchmark_mode), _absolute_mode(false), _kernel_regex(kernel_regex) {};
        // setters
        void set_tol(float tol) {_tol=tol;};
        void set_scalar(lv_32fc_t scalar) {_scalar=scalar;};
        void set_vlen(unsigned int vlen) {_vlen=vlen;};
        void set_iter(unsigned int iter) {_iter=iter;};
        void set_benchmark(bool benchmark) {_benchmark_mode=benchmark;};
        void set_regex(std::string regex) {_kernel_regex=regex;};
        // getters
        float tol() {return _tol;};
        lv_32fc_t scalar() {return _scalar;};
        unsigned int vlen() {return _vlen;};
        unsigned int iter() {return _iter;};
        bool benchmark_mode() {return _benchmark_mode;};
        bool absolute_mode() {return _absolute_mode;};
        std::string kernel_regex() {return _kernel_regex;};
        volk_test_params_t make_absolute(float tol) {
          volk_test_params_t t(*this);
          t._tol = tol;
          t._absolute_mode = true;
          return t;
        }
        volk_test_params_t make_tol(float tol) {
          volk_test_params_t t(*this);
          t._tol = tol;
          return t;
        }
};

class volk_test_case_t {
    private:
        volk_func_desc_t _desc;
        void(*_kernel_ptr)();
        std::string _name;
        volk_test_params_t _test_parameters;
        std::string _puppet_master_name;
    public:
        volk_func_desc_t desc() {return _desc;};
        void (*kernel_ptr()) () {return _kernel_ptr;};
        std::string name() {return _name;};
        std::string puppet_master_name() {return _puppet_master_name;};
        volk_test_params_t test_parameters() {return _test_parameters;};
        // normal ctor
        volk_test_case_t(volk_func_desc_t desc, void(*kernel_ptr)(), std::string name,
            volk_test_params_t test_parameters) :
            _desc(desc), _kernel_ptr(kernel_ptr), _name(name), _test_parameters(test_parameters),
            _puppet_master_name("NULL")
            {};
        // ctor for puppets
        volk_test_case_t(volk_func_desc_t desc, void(*kernel_ptr)(), std::string name,
            std::string puppet_master_name, volk_test_params_t test_parameters) :
            _desc(desc), _kernel_ptr(kernel_ptr), _name(name), _test_parameters(test_parameters),
            _puppet_master_name(puppet_master_name)
            {};
};

/************************************************
 * VOLK QA functions                            *
 ************************************************/
volk_type_t volk_type_from_string(std::string);

float uniform(void);
void random_floats(float *buf, unsigned n);

bool run_volk_tests(
    volk_func_desc_t,
    void(*)(),
    std::string,
    volk_test_params_t,
    std::vector<volk_test_results_t> *results = NULL,
    std::string puppet_master_name = "NULL"
    );

bool run_volk_tests(
        volk_func_desc_t,
        void(*)(),
        std::string,
        float,
        lv_32fc_t,
        unsigned int,
        unsigned int,
        std::vector<volk_test_results_t> *results = NULL,
        std::string puppet_master_name = "NULL",
        bool absolute_mode = false,
        bool benchmark_mode = false
);


#define VOLK_RUN_TESTS(func, tol, scalar, len, iter) \
    BOOST_AUTO_TEST_CASE(func##_test) { \
        BOOST_CHECK_EQUAL(run_volk_tests( \
            func##_get_func_desc(), (void (*)())func##_manual, \
            std::string(#func), tol, scalar, len, iter, 0, "NULL"), \
          0); \
    }
#define VOLK_PROFILE(func, test_params, results) run_volk_tests(func##_get_func_desc(), (void (*)())func##_manual, std::string(#func), test_params, results, "NULL")
#define VOLK_PUPPET_PROFILE(func, puppet_master_func, test_params, results) run_volk_tests(func##_get_func_desc(), (void (*)())func##_manual, std::string(#func), test_params, results, std::string(#puppet_master_func))
typedef void (*volk_fn_1arg)(void *, unsigned int, const char*); //one input, operate in place
typedef void (*volk_fn_2arg)(void *, void *, unsigned int, const char*);
typedef void (*volk_fn_3arg)(void *, void *, void *, unsigned int, const char*);
typedef void (*volk_fn_4arg)(void *, void *, void *, void *, unsigned int, const char*);
typedef void (*volk_fn_1arg_s32f)(void *, float, unsigned int, const char*); //one input vector, one scalar float input
typedef void (*volk_fn_2arg_s32f)(void *, void *, float, unsigned int, const char*);
typedef void (*volk_fn_3arg_s32f)(void *, void *, void *, float, unsigned int, const char*);
typedef void (*volk_fn_1arg_s32fc)(void *, lv_32fc_t, unsigned int, const char*); //one input vector, one scalar float input
typedef void (*volk_fn_2arg_s32fc)(void *, void *, lv_32fc_t, unsigned int, const char*);
typedef void (*volk_fn_3arg_s32fc)(void *, void *, void *, lv_32fc_t, unsigned int, const char*);

#endif //VOLK_QA_UTILS_H
