#include "core/RunnerSpatialState.h"
#include "core/RunnerBorrowedState.h"
#include <cassert>
#include <cstdio>
int main(){
    using namespace downwell;
    using Store=RunnerBorrowedSpatialStates<RunnerSpatialState,8>;
    static_assert(sizeof(Store)<sizeof(RunnerSpatialState)*2,"No hidden slot array");
    static_assert(sizeof(Store)==sizeof(RunnerBorrowedSpatialStates<RunnerSpatialState,65534>),
        "Unrelated slot capacity must not allocate reference records in the borrower");
    Store missing;
    bool owner=true;missing.observeFailure(owner);
    assert(missing.valid()&&!missing.read(0)&&!missing.valid());
    assert(!owner); // No per-frame validation scan needed to notice failure.
    missing.edit(7).get().flags=23;assert(!missing.valid());
    RunnerSpatialState backing[8]{};
    Store bound;assert(bound.bindStorage(backing));
    RunnerSpatialState value;value.flags=41;assert(value.x.assignInteger(17)&&value.y.assignInteger(-9));
    bound.fresh(2,value);assert(bound.valid()&&bound.read(2)==backing+2&&bound.flags(2)==41);
    assert(backing[2].previous.x.bits==value.x.bits&&backing[2].previous.y.bits==value.y.bits);
    assert(bound.edit(2).get().x.assignInteger(33)&&bound.capture(2));
    assert(backing[2].previous.x.bits==backing[2].x.bits);
    assert(!bound.read(8)&&!bound.valid());
    Store duplicate;assert(duplicate.bindStorage(backing));assert(!duplicate.bindStorage(backing)&&!duplicate.valid());
    Store absent;assert(!absent.bindStorage(nullptr)&&!absent.valid());
    std::puts("Borrowed scalar state: explicit binding, shared backing, bounds and terminal failure passed");
}
