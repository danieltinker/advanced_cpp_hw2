#include "MyPlayerFactory.h"  // include the project header

// Definition must match the 'const' qualifier in the declaration:
// std::unique_ptr<common::Player> arena::MyPlayerFactory::create(
//     int            player_index,
//     std::size_t    rows,
//     std::size_t    cols,
//     std::size_t    max_steps,
//     std::size_t    num_shells
// ) const {
//     if (player_index == 1) {
//         return std::make_unique<Player1>(
//             player_index, rows, cols, max_steps, num_shells
//         );
//     } else {
//         return std::make_unique<Player2>(
//             player_index, rows, cols, max_steps, num_shells
//         );
//     }
// }