cpp_include "extra_types.h"

cpp_namespace thrift.extra
php_namespace ThriftExtra

typedef map cpp_type "hash_map" <string, i32> StringCounts
typedef set cpp_type "hash_set" <i32> IntSet
typedef list<i32> cpp_type "std::deque" IntDeque

struct Widget {
  1: required i32 id,
  2: optional string label,
  3: StringCounts counts,
  4: IntSet tags,
  5: IntDeque history
}

service WidgetService {
  oneway void log(1: string message);
  async void ping();
}
