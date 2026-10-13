#pragma once
#include "CoreMinimal.h"
#include <atomic>

// Pure thread-safe ownership shared by UI and transport. No GT state or widget in I/O.
class FNodeForgeEditorBusinessGate
{
public:
    bool TryUI() { uint8 Expected = 0; return Owner.compare_exchange_strong(Expected, 1); }
    void EndUI() { uint8 Expected = 1; const bool Ended = Owner.compare_exchange_strong(Expected, 0); check(Ended); }
    bool ReserveControl() { uint8 Expected = 0; return Owner.compare_exchange_strong(Expected, 2); }
    bool BeginControl() { uint8 Expected = 2; return Owner.compare_exchange_strong(Expected, 3); }
    void EndControl()
    {
        uint8 Expected = 3;
        if (!Owner.compare_exchange_strong(Expected, 0)) { Expected = 2; const bool Ended = Owner.compare_exchange_strong(Expected, 0); check(Ended); }
    }
    bool IsBusy() const { return Owner.load() != 0; }
    bool IsControlReserved() const { return Owner.load() == 2; }
    bool IsUIRunning() const { return Owner.load() == 1; }
private:
    std::atomic<uint8> Owner{0};
};
