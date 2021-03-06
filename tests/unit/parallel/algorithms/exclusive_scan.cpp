//  Copyright (c) 2014-2015 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx_init.hpp>
#include <hpx/hpx.hpp>
#include <hpx/include/parallel_scan.hpp>
#include <hpx/util/lightweight_test.hpp>

#include <boost/iterator/counting_iterator.hpp>
#include <boost/range/functions.hpp>

#include <string>
#include <vector>

#include "test_utils.hpp"

///////////////////////////////////////////////////////////////////////////////
void exclusive_scan_benchmark()
{
    try {
      std::vector<double> c(100000000);
      std::vector<double> d(c.size());
      std::fill(boost::begin(c), boost::end(c), 1.0);

      double const val(0);
      auto op =
        [val](double v1, double v2) {
          return v1 + v2;
      };

      hpx::util::high_resolution_timer t;
      hpx::parallel::exclusive_scan(hpx::parallel::par,
        boost::begin(c), boost::end(c), boost::begin(d),
        val, op);
      double elapsed = t.elapsed();

      // verify values
      std::vector<double> e(c.size());
      hpx::parallel::v1::detail::sequential_exclusive_scan(
          boost::begin(c), boost::end(c), boost::begin(e), val, op);

      bool ok = std::equal(boost::begin(d), boost::end(d), boost::begin(e));
      HPX_TEST(ok);
      if (ok) {
          std::cout << "<DartMeasurement name=\"ExclusiveScanTime\" \n"
              << "type=\"numeric/double\">" << elapsed << "</DartMeasurement> \n";
      }
    }
    catch (...) {
      HPX_TEST(false);
    }
}

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_exclusive_scan1(ExPolicy policy, IteratorTag)
{
    static_assert(
        hpx::parallel::is_execution_policy<ExPolicy>::value,
        "hpx::parallel::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d(c.size());
    std::fill(boost::begin(c), boost::end(c), std::size_t(1));

    std::size_t const val(0);
    auto op =
        [val](std::size_t v1, std::size_t v2) {
            return v1 + v2;
        };

    hpx::parallel::exclusive_scan(policy,
        iterator(boost::begin(c)), iterator(boost::end(c)), boost::begin(d),
        val, op);

    // verify values
    std::vector<std::size_t> e(c.size());
    hpx::parallel::v1::detail::sequential_exclusive_scan(
        boost::begin(c), boost::end(c), boost::begin(e), val, op);

    HPX_TEST(std::equal(boost::begin(d), boost::end(d), boost::begin(e)));
}

template <typename ExPolicy, typename IteratorTag>
void test_exclusive_scan1_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d(c.size());
    std::fill(boost::begin(c), boost::end(c), std::size_t(1));

    std::size_t const val(0);
    auto op =
        [val](std::size_t v1, std::size_t v2) {
            return v1 + v2;
        };

    hpx::future<void> f =
        hpx::parallel::exclusive_scan(p,
            iterator(boost::begin(c)), iterator(boost::end(c)), boost::begin(d),
            val, op);
    f.wait();

    // verify values
    std::vector<std::size_t> e(c.size());
    hpx::parallel::v1::detail::sequential_exclusive_scan(
        boost::begin(c), boost::end(c), boost::begin(e), val, op);

    HPX_TEST(std::equal(boost::begin(d), boost::end(d), boost::begin(e)));
}

template <typename IteratorTag>
void test_exclusive_scan1()
{
    using namespace hpx::parallel;

    test_exclusive_scan1(seq, IteratorTag());
    test_exclusive_scan1(par, IteratorTag());
    test_exclusive_scan1(par_vec, IteratorTag());

    test_exclusive_scan1_async(seq(task), IteratorTag());
    test_exclusive_scan1_async(par(task), IteratorTag());

#if defined(HPX_HAVE_GENERIC_EXECUTION_POLICY)
    test_exclusive_scan1(execution_policy(seq), IteratorTag());
    test_exclusive_scan1(execution_policy(par), IteratorTag());
    test_exclusive_scan1(execution_policy(par_vec), IteratorTag());

    test_exclusive_scan1(execution_policy(seq(task)), IteratorTag());
    test_exclusive_scan1(execution_policy(par(task)), IteratorTag());
#endif
}

void exclusive_scan_test1()
{
    test_exclusive_scan1<std::random_access_iterator_tag>();
    test_exclusive_scan1<std::forward_iterator_tag>();
    test_exclusive_scan1<std::input_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(boost::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int)std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    // if benchmark is requested we run it even in debug mode
    if (vm.count("benchmark")) {
        exclusive_scan_benchmark();
    }
    else {
        exclusive_scan_test1();
#ifndef HPX_DEBUG
        exclusive_scan_benchmark();
#endif
    }

  return hpx::finalize();
}

int main(int argc, char* argv[])
{
    // add command line option which controls the random number generator seed
    using namespace boost::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()
        ("seed,s", value<unsigned int>(),
        "the random number generator seed to use for this run")
        ;
    // By default this test should run on all available cores
    std::vector<std::string> const cfg = {
        "hpx.os_threads=all"
    };

    // Initialize and run HPX
    HPX_TEST_EQ_MSG(hpx::init(desc_commandline, argc, argv, cfg), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
