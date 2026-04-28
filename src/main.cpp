#include <Geode/Geode.hpp>

#include "IdleTracker.hpp"

$on_mod(Loaded) {
    gdss::IdleTracker::get().markInput();
}
