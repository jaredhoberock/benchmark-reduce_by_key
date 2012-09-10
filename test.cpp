#include <thrust/device_vector.h>
#include <thrust/transform.h>
#include <thrust/reduce.h>
#include <thrust/iterator/zip_iterator.h>
#include <thrust/pair.h>
#include "time_invocation_cpp03.hpp"
#include <cassert>

typedef thrust::pair<
 thrust::device_vector<int>::pointer,
 thrust::device_vector<int>::pointer
> segment_type;

void do_it_nested(const thrust::device_vector<int> &input_keys,
                  const thrust::device_vector<segment_type> &input_value_segments,
                  thrust::device_vector<int> &output_keys,
                  thrust::device_vector<int> &output_values)
{
  using namespace thrust;

  transform(input_keys.begin(), input_keys.end(), input_value_segments.begin(), make_zip_iterator(make_tuple(output_keys.begin(), output_values.begin())),
    [](int key, segment_type val_seg) {
      return make_tuple(key, reduce(val_seg.first, val_seg.second));
    }
  );
}

void do_it(const thrust::device_vector<int> &input_keys,
           const thrust::device_vector<int> &input_values,
           thrust::device_vector<int> &output_keys,
           thrust::device_vector<int> &output_values)
{
  using namespace thrust;

  reduce_by_key(input_keys.begin(), input_keys.end(), input_values.begin(), output_keys.begin(), output_values.begin());
}

thrust::pair<float,float> test(size_t log_n, size_t log_segment_size)
{
  assert(log_segment_size <= log_n);

  size_t n = 1 << log_n;
  size_t segment_size = 1 << log_segment_size;
  size_t num_segments = n / segment_size;

  // init input
  thrust::device_vector<int> keys(n);
  thrust::device_vector<int> values(n);
  for(int i = 0; i < keys.size(); ++i)
  {
    keys[i] = i / segment_size;
  }
  thrust::fill(values.begin(), values.end(), 13);

  // allocate output
  thrust::device_vector<int> output_keys(n / segment_size);
  thrust::device_vector<int> output_values(n / segment_size);

  typedef thrust::device_vector<int>::pointer ptr;

  // generate segments for nested call
  thrust::device_vector<int>                   decimated_keys(num_segments);
  thrust::device_vector<thrust::pair<ptr,ptr>> value_segments(num_segments);
  for(int i = 0; i < num_segments; ++i)
  {
    decimated_keys[i] = i;

    int this_seg = i * segment_size;
    int next_seg = this_seg + segment_size;

    auto first = &values[this_seg];
    auto second = &values[next_seg];

    value_segments[i] = thrust::make_pair(first, second);
  }

  thrust::device_vector<int> output_keys_ref(n / segment_size);
  thrust::device_vector<int> output_values_ref(n / segment_size);

  // generate reference
  do_it(keys, values, output_keys_ref, output_values_ref);

  // validate
  do_it_nested(decimated_keys, value_segments, output_keys, output_values);

  assert(output_keys_ref == output_keys);
  assert(output_values_ref == output_values);

  auto reduce_by_key_time = time_invocation(20, do_it, keys, values, output_keys_ref, output_values_ref);

  auto nested_time = time_invocation(20, do_it_nested, decimated_keys, value_segments, output_keys, output_values);

  return thrust::make_pair(reduce_by_key_time, nested_time);
}

int main()
{
  size_t log_n = 24;

  std::cout << "segment size, reduce_by_key GK/s, nested GK/s" << std::endl;

  for(int log_segment_size = 0; log_segment_size <= log_n; ++log_segment_size)
  {
    float reduce_by_key_msec, nested_msec;
    thrust::tie(reduce_by_key_msec, nested_msec) = test(log_n, log_segment_size);

    auto reduce_by_key_seconds = reduce_by_key_msec / 1000;
    auto nested_seconds = nested_msec / 1000;

    auto reduce_by_key_keys_per_second = float(1 << log_n) / reduce_by_key_seconds;
    auto reduce_by_key_gkeys_per_second = reduce_by_key_keys_per_second / (1 << 30);

    auto nested_keys_per_second = float(1 << log_n) / nested_seconds;
    auto nested_gkeys_per_second = nested_keys_per_second / (1 << 30);

    std::cout << (1<<log_segment_size) << ", " << reduce_by_key_gkeys_per_second << ", " << nested_gkeys_per_second << std::endl;
  }

  return 0;
}

