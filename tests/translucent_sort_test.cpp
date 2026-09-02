#include "pc_endjinn_translucent_sort.h"

#include <cassert>
#include <cstdio>
#include <vector>

using pc_endjinn_pvr::QueuedPrimitive;
namespace translucent_sort = pc_endjinn_translucent_sort;

namespace {

QueuedPrimitive triangle(float ax, float ay, float az,
                         float bx, float by, float bz,
                         float cx, float cy, float cz) {
  QueuedPrimitive primitive{};
  primitive.count = 3u;
  primitive.x[0] = ax;
  primitive.y[0] = ay;
  primitive.z[0] = az;
  primitive.x[1] = bx;
  primitive.y[1] = by;
  primitive.z[1] = bz;
  primitive.x[2] = cx;
  primitive.y[2] = cy;
  primitive.z[2] = cz;
  primitive.culling = PVR_CULLING_NONE;
  return primitive;
}

void test_overlapping_depth_order() {
  const QueuedPrimitive far = triangle(
      0.0f, 0.0f, 0.2f, 2.0f, 0.0f, 0.2f, 0.0f, 2.0f, 0.2f);
  const QueuedPrimitive near = triangle(
      0.0f, 0.0f, 0.8f, 2.0f, 0.0f, 0.8f, 0.0f, 2.0f, 0.8f);
  assert(translucent_sort::overlapping_depth_order(far, near) == -1);
  assert(translucent_sort::overlapping_depth_order(near, far) == 1);

  std::vector<const QueuedPrimitive *> primitives = {&near, &far};
  const translucent_sort::Diagnostics diagnostics =
      translucent_sort::sort(primitives);
  assert(primitives[0] == &far);
  assert(primitives[1] == &near);
  assert(diagnostics.candidate_pairs == 1u);
  assert(diagnostics.dependency_edges == 1u);
  assert(diagnostics.cycle_breaks == 0u);
}

void test_disjoint_and_shared_edges_are_unconstrained() {
  const QueuedPrimitive left = triangle(
      0.0f, 0.0f, 0.2f, 1.0f, 0.0f, 0.2f, 0.0f, 1.0f, 0.2f);
  const QueuedPrimitive right = triangle(
      2.0f, 0.0f, 0.8f, 3.0f, 0.0f, 0.8f, 2.0f, 1.0f, 0.8f);
  assert(translucent_sort::overlapping_depth_order(left, right) == 0);

  const QueuedPrimitive shared = triangle(
      1.0f, 0.0f, 0.2f, 1.0f, 1.0f, 0.2f, 0.0f, 1.0f, 0.2f);
  assert(translucent_sort::overlapping_depth_order(left, shared) == 0);
}

void test_intersecting_depth_planes_are_unordered() {
  const QueuedPrimitive sloped = triangle(
      0.0f, 0.0f, 0.2f, 2.0f, 0.0f, 0.8f, 0.0f, 2.0f, 0.2f);
  const QueuedPrimitive flat = triangle(
      0.0f, 0.0f, 0.5f, 2.0f, 0.0f, 0.5f, 0.0f, 2.0f, 0.5f);
  assert(translucent_sort::overlapping_depth_order(sloped, flat) == 0);

  std::vector<const QueuedPrimitive *> primitives = {&sloped, &flat};
  const translucent_sort::Diagnostics diagnostics =
      translucent_sort::sort(primitives);
  assert(diagnostics.candidate_pairs == 1u);
  assert(diagnostics.dependency_edges == 0u);
  assert(diagnostics.unordered_pairs == 1u);
}

void test_cycle_break_is_deterministic() {
  const std::vector<float> average_depth = {0.3f, 0.2f, 0.1f};
  const std::vector<std::vector<size_t>> outgoing = {{1u}, {2u}, {0u}};
  translucent_sort::Diagnostics diagnostics{};
  const std::vector<size_t> order =
      translucent_sort::detail::sort_dependency_graph(
          average_depth, outgoing, diagnostics);
  assert((order == std::vector<size_t>{2u, 0u, 1u}));
  assert(diagnostics.cycle_breaks == 1u);
}

void test_pathological_count_uses_bounded_fallback() {
  const QueuedPrimitive primitive = triangle(
      0.0f, 0.0f, 0.5f, 1.0f, 0.0f, 0.5f, 0.0f, 1.0f, 0.5f);
  std::vector<const QueuedPrimitive *> primitives(2049u, &primitive);
  const translucent_sort::Diagnostics diagnostics =
      translucent_sort::sort(primitives);
  assert(diagnostics.primitive_count == 2049u);
  assert(diagnostics.average_depth_fallback);
  assert(diagnostics.candidate_pairs == 0u);
}

}  // namespace

int main() {
  test_overlapping_depth_order();
  test_disjoint_and_shared_edges_are_unconstrained();
  test_intersecting_depth_planes_are_unordered();
  test_cycle_break_is_deterministic();
  test_pathological_count_uses_bounded_fallback();
  std::puts("translucent_sort_test: ok");
  return 0;
}
