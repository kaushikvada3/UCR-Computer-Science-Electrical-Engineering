#include "Jug.h"

#include <iostream>

int main()
{
    std::string solution;
    Jug head(3, 5, 4, 1, 2, 3, 4, 5, 6);

    if (head.solve(solution) != 1)
    {
        std::cout << "Error 3" << std::endl;
    }

    std::cout << solution << std::endl << std::endl;

    std::string solution2;
    Jug head2(3, 5, 4, 1, 1, 1, 1, 1, 2);
    
    if (head2.solve(solution2) != 1)
    {
        std::cout << "Error 3" << std::endl;
    }

    std::cout << solution2 << std::endl;

    return 0;
}