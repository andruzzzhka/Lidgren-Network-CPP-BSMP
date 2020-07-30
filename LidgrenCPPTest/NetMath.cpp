
#include "NetMath.h"

namespace LidgrenNetwork
{

    int NetMath::RoundIntegerUp(unsigned int numToRound, int multiple)
    {
        if (multiple == 0)
            return numToRound;

        int remainder = numToRound % multiple;
        if (remainder == 0)
            return numToRound;

        return numToRound + multiple - remainder;
    }
}