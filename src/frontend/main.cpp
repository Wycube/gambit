#include "common/Version.hpp"
#include "common/Log.hpp"


int main() {
    printf("Version: %s\n", common::GIT_DESC);
    printf("Commit: %s\n", common::GIT_COMMIT);
    printf("Branch: %s\n", common::GIT_BRANCH);
}