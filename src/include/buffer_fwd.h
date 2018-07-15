#ifndef BUFFER_FWD_H
#define BUFFER_FWD_H

namespace ceph {
  namespace buffer {
  /* 命名空间,在该命名空间下定义了Buffer相关的数据结构 */
    class ptr;
    class list;
    class hash;
  }

  using bufferptr = buffer::ptr;
  using bufferlist = buffer::list;
  using bufferhash = buffer::hash;
}

#endif

