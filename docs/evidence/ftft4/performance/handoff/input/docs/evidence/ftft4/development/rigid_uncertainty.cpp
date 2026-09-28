// Extra diagnostic around the unchanged real rigid suite, not a replacement.
#include "Contacts.h"
#define main OriginalRigidContactMain
#include "../../../../tests/RigidContactTests.cpp"
#undef main
int main() {
    ResetContactGeometryDiagnostics();
    const int originalResult=OriginalRigidContactMain();
    const auto d=GetContactGeometryDiagnostics();
    std::printf("FTFT4A rigid uncertainty predicates=%llu interval=%llu exact=%llu unresolved=%llu invalid=%llu\n",
        (unsigned long long)d.predicates,(unsigned long long)d.intervalResolved,
        (unsigned long long)d.exactFallbacks,(unsigned long long)d.unresolved,
        (unsigned long long)d.invalidInputs);
    return originalResult || d.unresolved || d.invalidInputs ? 1 : 0;
}
