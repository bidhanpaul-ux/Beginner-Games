#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <limits>
#include <algorithm>
#include <cctype>
#include <chrono>

using namespace std;

// 
namespace Color {
    const string RESET  = "\033[0m";
    const string BOLD   = "\033[1m";
    const string WHITE_P= "\033[1;97m";   // white pieces
    const string BLACK_P= "\033[1;34m";   // black pieces
    const string LIGHT  = "\033[47m";     // light square background
    const string DARK   = "\033[100m";    // dark square background
    const string GREEN  = "\033[1;32m";
    const string RED    = "\033[1;31m";
    const string YELLOW = "\033[1;33m";
    const string GREY   = "\033[0;90m";
}

//
enum class PieceType { NONE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };
enum class PColor { NONE, WHITE, BLACK };

struct Piece {
    PieceType type = PieceType::NONE;
    PColor color = PColor::NONE;
    bool isEmpty() const { return type == PieceType::NONE; }
};

inline PColor opponent(PColor c) {
    return c == PColor::WHITE ? PColor::BLACK : PColor::WHITE;
}

char pieceLetter(PieceType t) {
    switch (t) {
        case PieceType::PAWN:   return 'P';
        case PieceType::KNIGHT: return 'N';
        case PieceType::BISHOP: return 'B';
        case PieceType::ROOK:   return 'R';
        case PieceType::QUEEN:  return 'Q';
        case PieceType::KING:   return 'K';
        default: return '.';
    }
}

//
struct Move {
    int fr, fc, tr, tc;                          // from-row/col, to-row/col
    PieceType promotion = PieceType::NONE;       // set if this move promotes a pawn
    bool isCastleK = false, isCastleQ = false;   // castling flags
    bool isEnPassant = false;                    // en passant capture flag
};

// Undo information so the AI can make/unmake moves during search without copying the whole board.
struct UndoInfo {
    Piece captured;                 // piece that stood on the destination square (if any)
    Piece capturedEnPassant;        // pawn removed via en passant (if any)
    bool prevCastleWK, prevCastleWQ, prevCastleBK, prevCastleBQ;
    int prevEpRow, prevEpCol;
};

// Converts board coordinates to algebraic notation, e.g. (0,4) -> "e1"
string squareName(int r, int c) {
    string s;
    s += char('a' + c);
    s += char('1' + r);
    return s;
}

// ============================================================================
//  BOARD CLASS
//  Holds the 8x8 grid plus game state (castling rights, en passant target)
//  and all move-generation / rule logic.
// ============================================================================
class Board {
public:
    Board() { setupStartPosition(); }

    void setupStartPosition() {
        for (auto& row : grid) row.fill(Piece{});

        auto place = [&](int r, int c, PieceType t, PColor col) {
            grid[r][c] = Piece{t, col};
        };

        // Pawns
        for (int c = 0; c < 8; ++c) {
            place(1, c, PieceType::PAWN, PColor::WHITE);
            place(6, c, PieceType::PAWN, PColor::BLACK);
        }
        // Back ranks
        PieceType order[8] = {
            PieceType::ROOK, PieceType::KNIGHT, PieceType::BISHOP, PieceType::QUEEN,
            PieceType::KING, PieceType::BISHOP, PieceType::KNIGHT, PieceType::ROOK
        };
        for (int c = 0; c < 8; ++c) {
            place(0, c, order[c], PColor::WHITE);
            place(7, c, order[c], PColor::BLACK);
        }

        castleWK = castleWQ = castleBK = castleBQ = true;
        epRow = epCol = -1;
    }

    static bool inBounds(int r, int c) { return r >= 0 && r < 8 && c >= 0 && c < 8; }

    Piece at(int r, int c) const { return grid[r][c]; }

    // ------------------------------------------------------------------
    //  Attack detection — used for check detection & castling safety.
    //  Does NOT depend on legal-move filtering, so it's safe to call
    //  from inside legal-move generation without infinite recursion.
    // ------------------------------------------------------------------
    bool isSquareAttacked(int r, int c, PColor byColor) const {
        // Pawn attacks
        int pawnDir = (byColor == PColor::WHITE) ? -1 : 1; // attacker's pawn sits "behind" relative to target
        for (int dc : {-1, 1}) {
            int pr = r + pawnDir, pc = c + dc;
            if (inBounds(pr, pc)) {
                Piece p = grid[pr][pc];
                if (p.type == PieceType::PAWN && p.color == byColor) return true;
            }
        }

        // Knight attacks
        static const int kOff[8][2] = {
            {1,2},{2,1},{-1,2},{-2,1},{1,-2},{2,-1},{-1,-2},{-2,-1}
        };
        for (auto& o : kOff) {
            int nr = r + o[0], nc = c + o[1];
            if (inBounds(nr, nc)) {
                Piece p = grid[nr][nc];
                if (p.type == PieceType::KNIGHT && p.color == byColor) return true;
            }
        }

        // Sliding pieces: bishop/queen on diagonals, rook/queen on straights
        static const int diagDirs[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
        for (auto& d : diagDirs) {
            int nr = r + d[0], nc = c + d[1];
            while (inBounds(nr, nc)) {
                Piece p = grid[nr][nc];
                if (!p.isEmpty()) {
                    if (p.color == byColor && (p.type == PieceType::BISHOP || p.type == PieceType::QUEEN))
                        return true;
                    break; // blocked
                }
                nr += d[0]; nc += d[1];
            }
        }
        static const int straightDirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
        for (auto& d : straightDirs) {
            int nr = r + d[0], nc = c + d[1];
            while (inBounds(nr, nc)) {
                Piece p = grid[nr][nc];
                if (!p.isEmpty()) {
                    if (p.color == byColor && (p.type == PieceType::ROOK || p.type == PieceType::QUEEN))
                        return true;
                    break;
                }
                nr += d[0]; nc += d[1];
            }
        }

        // King attacks (adjacent squares)
        for (int dr = -1; dr <= 1; ++dr) {
            for (int dc = -1; dc <= 1; ++dc) {
                if (dr == 0 && dc == 0) continue;
                int nr = r + dr, nc = c + dc;
                if (inBounds(nr, nc)) {
                    Piece p = grid[nr][nc];
                    if (p.type == PieceType::KING && p.color == byColor) return true;
                }
            }
        }

        return false;
    }

    pair<int,int> findKing(PColor c) const {
        for (int r = 0; r < 8; ++r)
            for (int col = 0; col < 8; ++col)
                if (grid[r][col].type == PieceType::KING && grid[r][col].color == c)
                    return {r, col};
        return {-1, -1};
    }

    bool inCheck(PColor c) const {
        auto [kr, kc] = findKing(c);
        if (kr == -1) return false; // shouldn't happen in a normal game
        return isSquareAttacked(kr, kc, opponent(c));
    }

    // ------------------------------------------------------------------
    //  Pseudo-legal move generation (ignores whether the mover's own
    //  king ends up in check — that filtering happens in legalMoves()).
    // ------------------------------------------------------------------
    vector<Move> pseudoLegalMoves(PColor side) const {
        vector<Move> moves;
        moves.reserve(48);

        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                Piece p = grid[r][c];
                if (p.isEmpty() || p.color != side) continue;

                switch (p.type) {
                    case PieceType::PAWN:   genPawnMoves(r, c, side, moves); break;
                    case PieceType::KNIGHT: genKnightMoves(r, c, side, moves); break;
                    case PieceType::BISHOP: genSlidingMoves(r, c, side, moves, true, false); break;
                    case PieceType::ROOK:   genSlidingMoves(r, c, side, moves, false, true); break;
                    case PieceType::QUEEN:  genSlidingMoves(r, c, side, moves, true, true); break;
                    case PieceType::KING:   genKingMoves(r, c, side, moves); break;
                    default: break;
                }
            }
        }
        return moves;
    }

    // Only truly legal moves (own king safe afterwards).
    vector<Move> legalMoves(PColor side) {
        vector<Move> pseudo = pseudoLegalMoves(side);
        vector<Move> legal;
        legal.reserve(pseudo.size());

        for (auto& m : pseudo) {
            UndoInfo u = makeMove(m);
            if (!inCheck(side)) legal.push_back(m);
            undoMove(m, u);
        }
        return legal;
    }

    bool isCheckmate(PColor side) { return inCheck(side) && legalMoves(side).empty(); }
    bool isStalemate(PColor side) { return !inCheck(side) && legalMoves(side).empty(); }

    // ------------------------------------------------------------------
    //  Apply / undo a move on the board (used by both the human-move
    //  path and the AI's search, so search never needs to copy the board).
    // ------------------------------------------------------------------
    UndoInfo makeMove(const Move& m) {
        UndoInfo u;
        u.prevCastleWK = castleWK; u.prevCastleWQ = castleWQ;
        u.prevCastleBK = castleBK; u.prevCastleBQ = castleBQ;
        u.prevEpRow = epRow; u.prevEpCol = epCol;
        u.captured = grid[m.tr][m.tc];
        u.capturedEnPassant = Piece{};

        Piece moving = grid[m.fr][m.fc];

        if (m.isEnPassant) {
            // The captured pawn sits on the same row as the mover started,
            // in the destination column.
            u.capturedEnPassant = grid[m.fr][m.tc];
            grid[m.fr][m.tc] = Piece{};
        }

        // Move the piece
        grid[m.tr][m.tc] = moving;
        grid[m.fr][m.fc] = Piece{};

        // Promotion
        if (m.promotion != PieceType::NONE) {
            grid[m.tr][m.tc].type = m.promotion;
        }

        // Castling rook relocation
        if (m.isCastleK) {
            int r = m.fr;
            grid[r][5] = grid[r][7];
            grid[r][7] = Piece{};
        }
        if (m.isCastleQ) {
            int r = m.fr;
            grid[r][3] = grid[r][0];
            grid[r][0] = Piece{};
        }

        // Update castling rights
        if (moving.type == PieceType::KING) {
            if (moving.color == PColor::WHITE) { castleWK = false; castleWQ = false; }
            else                                { castleBK = false; castleBQ = false; }
        }
        if (moving.type == PieceType::ROOK) {
            if (m.fr == 0 && m.fc == 0) castleWQ = false;
            if (m.fr == 0 && m.fc == 7) castleWK = false;
            if (m.fr == 7 && m.fc == 0) castleBQ = false;
            if (m.fr == 7 && m.fc == 7) castleBK = false;
        }
        // If a rook gets captured on its home square, lose that right too
        if (m.tr == 0 && m.tc == 0) castleWQ = false;
        if (m.tr == 0 && m.tc == 7) castleWK = false;
        if (m.tr == 7 && m.tc == 0) castleBQ = false;
        if (m.tr == 7 && m.tc == 7) castleBK = false;

        // Update en passant target square
        epRow = epCol = -1;
        if (moving.type == PieceType::PAWN && abs(m.tr - m.fr) == 2) {
            epRow = (m.fr + m.tr) / 2;
            epCol = m.fc;
        }

        return u;
    }

    void undoMove(const Move& m, const UndoInfo& u) {
        Piece moved = grid[m.tr][m.tc];

        // Undo promotion (turn the piece back into a pawn before moving it back)
        if (m.promotion != PieceType::NONE) {
            moved.type = PieceType::PAWN;
        }

        grid[m.fr][m.fc] = moved;
        grid[m.tr][m.tc] = u.captured;

        if (m.isEnPassant) {
            grid[m.fr][m.tc] = u.capturedEnPassant;
        }

        if (m.isCastleK) {
            int r = m.fr;
            grid[r][7] = grid[r][5];
            grid[r][5] = Piece{};
        }
        if (m.isCastleQ) {
            int r = m.fr;
            grid[r][0] = grid[r][3];
            grid[r][3] = Piece{};
        }

        castleWK = u.prevCastleWK; castleWQ = u.prevCastleWQ;
        castleBK = u.prevCastleBK; castleBQ = u.prevCastleBQ;
        epRow = u.prevEpRow; epCol = u.prevEpCol;
    }

    // ------------------------------------------------------------------
    //  Rendering
    // ------------------------------------------------------------------
    void render() const {
        cout << "\n";
        for (int r = 7; r >= 0; --r) {
            cout << " " << (r + 1) << " ";
            for (int c = 0; c < 8; ++c) {
                bool light = ((r + c) % 2 == 1);
                string bg = light ? Color::LIGHT : Color::DARK;
                Piece p = grid[r][c];
                string glyph = "  ";
                if (!p.isEmpty()) {
                    string pcol = (p.color == PColor::WHITE) ? Color::WHITE_P : Color::BLACK_P;
                    glyph = pcol + string(1, pieceLetter(p.type)) + " " + Color::RESET;
                }
                cout << bg << " " << glyph << Color::RESET;
            }
            cout << "\n";
        }
        cout << "   ";
        for (char f = 'a'; f <= 'h'; ++f) cout << " " << f << "  ";
        cout << "\n";
    }

    bool castleWK, castleWQ, castleBK, castleBQ;
    int epRow, epCol;

private:
    array<array<Piece, 8>, 8> grid;

    void genPawnMoves(int r, int c, PColor side, vector<Move>& moves) const {
        int dir = (side == PColor::WHITE) ? 1 : -1;
        int startRow = (side == PColor::WHITE) ? 1 : 6;
        int promoRow = (side == PColor::WHITE) ? 7 : 0;

        // Single push
        int r1 = r + dir;
        if (inBounds(r1, c) && grid[r1][c].isEmpty()) {
            addPawnMoveWithPromotion(r, c, r1, c, promoRow, moves);

            // Double push from start row
            int r2 = r + 2 * dir;
            if (r == startRow && grid[r2][c].isEmpty()) {
                Move m{r, c, r2, c};
                moves.push_back(m);
            }
        }

        // Captures (including en passant)
        for (int dc : {-1, 1}) {
            int nc = c + dc;
            if (!inBounds(r1, nc)) continue;
            Piece target = grid[r1][nc];
            if (!target.isEmpty() && target.color == opponent(side)) {
                addPawnMoveWithPromotion(r, c, r1, nc, promoRow, moves);
            } else if (target.isEmpty() && r1 == epRow && nc == epCol) {
                Move m{r, c, r1, nc};
                m.isEnPassant = true;
                moves.push_back(m);
            }
        }
    }

    void addPawnMoveWithPromotion(int fr, int fc, int tr, int tc, int promoRow, vector<Move>& moves) const {
        if (tr == promoRow) {
            for (PieceType promo : {PieceType::QUEEN, PieceType::ROOK, PieceType::BISHOP, PieceType::KNIGHT}) {
                Move m{fr, fc, tr, tc};
                m.promotion = promo;
                moves.push_back(m);
            }
        } else {
            moves.push_back(Move{fr, fc, tr, tc});
        }
    }

    void genKnightMoves(int r, int c, PColor side, vector<Move>& moves) const {
        static const int off[8][2] = {
            {1,2},{2,1},{-1,2},{-2,1},{1,-2},{2,-1},{-1,-2},{-2,-1}
        };
        for (auto& o : off) {
            int nr = r + o[0], nc = c + o[1];
            if (!inBounds(nr, nc)) continue;
            Piece target = grid[nr][nc];
            if (target.isEmpty() || target.color != side) {
                moves.push_back(Move{r, c, nr, nc});
            }
        }
    }

    void genSlidingMoves(int r, int c, PColor side, vector<Move>& moves, bool diag, bool straight) const {
        vector<pair<int,int>> dirs;
        if (diag) { dirs.push_back({1,1}); dirs.push_back({1,-1}); dirs.push_back({-1,1}); dirs.push_back({-1,-1}); }
        if (straight) { dirs.push_back({1,0}); dirs.push_back({-1,0}); dirs.push_back({0,1}); dirs.push_back({0,-1}); }

        for (auto& d : dirs) {
            int nr = r + d.first, nc = c + d.second;
            while (inBounds(nr, nc)) {
                Piece target = grid[nr][nc];
                if (target.isEmpty()) {
                    moves.push_back(Move{r, c, nr, nc});
                } else {
                    if (target.color != side) moves.push_back(Move{r, c, nr, nc});
                    break;
                }
                nr += d.first; nc += d.second;
            }
        }
    }

    void genKingMoves(int r, int c, PColor side, vector<Move>& moves) const {
        for (int dr = -1; dr <= 1; ++dr) {
            for (int dc = -1; dc <= 1; ++dc) {
                if (dr == 0 && dc == 0) continue;
                int nr = r + dr, nc = c + dc;
                if (!inBounds(nr, nc)) continue;
                Piece target = grid[nr][nc];
                if (target.isEmpty() || target.color != side) {
                    moves.push_back(Move{r, c, nr, nc});
                }
            }
        }

        // Castling
        PColor opp = opponent(side);
        int homeRow = (side == PColor::WHITE) ? 0 : 7;
        if (r == homeRow && c == 4 && !isSquareAttacked(r, c, opp)) {
            bool kingsideRight = (side == PColor::WHITE) ? castleWK : castleBK;
            bool queensideRight = (side == PColor::WHITE) ? castleWQ : castleBQ;

            if (kingsideRight &&
                grid[homeRow][5].isEmpty() && grid[homeRow][6].isEmpty() &&
                !isSquareAttacked(homeRow, 5, opp) && !isSquareAttacked(homeRow, 6, opp)) {
                Move m{r, c, homeRow, 6};
                m.isCastleK = true;
                moves.push_back(m);
            }
            if (queensideRight &&
                grid[homeRow][1].isEmpty() && grid[homeRow][2].isEmpty() && grid[homeRow][3].isEmpty() &&
                !isSquareAttacked(homeRow, 3, opp) && !isSquareAttacked(homeRow, 2, opp)) {
                Move m{r, c, homeRow, 2};
                m.isCastleQ = true;
                moves.push_back(m);
            }
        }
    }
};

// ============================================================================
//  AI CLASS
//  Minimax search with Alpha-Beta pruning over the Board's legal moves.
// ============================================================================
class ChessAI {
public:
    ChessAI(PColor aiColor, int depth) : aiColor(aiColor), searchDepth(depth) {}

    Move chooseMove(Board& board) {
        vector<Move> moves = board.legalMoves(aiColor);
        orderMoves(board, moves);

        int bestScore = numeric_limits<int>::min();
        Move bestMove = moves.front();

        int alpha = numeric_limits<int>::min();
        int beta  = numeric_limits<int>::max();

        for (auto& m : moves) {
            UndoInfo u = board.makeMove(m);
            int score = minimax(board, searchDepth - 1, opponent(aiColor), alpha, beta);
            board.undoMove(m, u);

            if (score > bestScore) {
                bestScore = score;
                bestMove = m;
            }
            alpha = max(alpha, bestScore);
        }
        return bestMove;
    }

private:
    PColor aiColor;
    int searchDepth;

    int pieceValue(PieceType t) const {
        switch (t) {
            case PieceType::PAWN:   return 100;
            case PieceType::KNIGHT: return 320;
            case PieceType::BISHOP: return 330;
            case PieceType::ROOK:   return 500;
            case PieceType::QUEEN:  return 900;
            case PieceType::KING:   return 20000;
            default: return 0;
        }
    }

    // Positive = good for the AI, negative = good for the opponent.
    int evaluate(Board& board) {
        int score = 0;
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                Piece p = board.at(r, c);
                if (p.isEmpty()) continue;
                int value = pieceValue(p.type);
                score += (p.color == aiColor) ? value : -value;
            }
        }

        // Small mobility bonus encourages active piece play over passivity.
        int myMoves = (int)board.legalMoves(aiColor).size();
        int oppMoves = (int)board.legalMoves(opponent(aiColor)).size();
        score += (myMoves - oppMoves) * 2;

        return score;
    }

    // Puts capturing moves first so Alpha-Beta prunes more branches early.
    void orderMoves(Board& board, vector<Move>& moves) {
        sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
            bool aCap = !board.at(a.tr, a.tc).isEmpty() || a.isEnPassant;
            bool bCap = !board.at(b.tr, b.tc).isEmpty() || b.isEnPassant;
            return aCap && !bCap;
        });
    }

    int minimax(Board& board, int depth, PColor sideToMove, int alpha, int beta) {
        bool inCheck = board.inCheck(sideToMove);
        vector<Move> moves = board.legalMoves(sideToMove);

        if (moves.empty()) {
            if (inCheck) {
                // Checkmate — heavily penalise/reward, adjusted by depth so
                // faster mates are preferred over slower ones.
                int mateScore = 100000 + depth;
                return (sideToMove == aiColor) ? -mateScore : mateScore;
            }
            return 0; // stalemate = draw
        }

        if (depth == 0) return evaluate(board);

        orderMoves(board, moves);
        bool maximizing = (sideToMove == aiColor);

        if (maximizing) {
            int best = numeric_limits<int>::min();
            for (auto& m : moves) {
                UndoInfo u = board.makeMove(m);
                best = max(best, minimax(board, depth - 1, opponent(sideToMove), alpha, beta));
                board.undoMove(m, u);
                alpha = max(alpha, best);
                if (beta <= alpha) break;
            }
            return best;
        } else {
            int best = numeric_limits<int>::max();
            for (auto& m : moves) {
                UndoInfo u = board.makeMove(m);
                best = min(best, minimax(board, depth - 1, opponent(sideToMove), alpha, beta));
                board.undoMove(m, u);
                beta = min(beta, best);
                if (beta <= alpha) break;
            }
            return best;
        }
    }
};

// ============================================================================
//  GAME CLASS
//  Handles setup, the turn loop, move parsing/validation, and end conditions.
// ============================================================================
class Game {
public:
    void run() {
        printBanner();
        setup();

        Board board;
        PColor turn = PColor::WHITE;

        while (true) {
            board.render();

            if (board.isCheckmate(turn)) {
                cout << Color::BOLD << (turn == PColor::WHITE ? "Black" : "White")
                     << " wins by checkmate!\n" << Color::RESET;
                break;
            }
            if (board.isStalemate(turn)) {
                cout << Color::YELLOW << "Draw by stalemate!\n" << Color::RESET;
                break;
            }
            if (board.inCheck(turn)) {
                cout << Color::RED << (turn == PColor::WHITE ? "White" : "Black")
                     << " is in check!\n" << Color::RESET;
            }

            if (turn == humanColor) {
                Move m = readHumanMove(board, turn);
                board.makeMove(m);
                cout << "You played " << squareName(m.fr, m.fc) << squareName(m.tr, m.tc) << "\n";
            } else {
                cout << Color::YELLOW << "Computer is thinking..." << Color::RESET;
                cout.flush();
                Move m = ai->chooseMove(board);
                board.makeMove(m);
                cout << "\rComputer played " << squareName(m.fr, m.fc)
                     << squareName(m.tr, m.tc) << "        \n";
            }

            turn = opponent(turn);
        }
    }

private:
    PColor humanColor = PColor::WHITE;
    ChessAI* ai = nullptr;

    void setup() {
        cout << "Play as:\n  1) White (moves first)\n  2) Black\n> ";
        int c = readInt(1, 2);
        humanColor = (c == 1) ? PColor::WHITE : PColor::BLACK;

        cout << "\nChoose difficulty:\n"
                "  1) Easy   (looks 2 moves ahead)\n"
                "  2) Medium (looks 3 moves ahead)\n"
                "  3) Hard   (looks 4 moves ahead — plays strong tactical chess)\n> ";
        int d = readInt(1, 3);
        int depth = (d == 1) ? 2 : (d == 2) ? 3 : 4;

        ai = new ChessAI(opponent(humanColor), depth);

        cout << "\nEnter moves like " << Color::BOLD << "e2e4" << Color::RESET
             << " (from-square + to-square).\n"
             << "For pawn promotion, append the piece letter, e.g. "
             << Color::BOLD << "e7e8q" << Color::RESET << " (q/r/b/n).\n";
    }

    int readInt(int lo, int hi) {
        while (true) {
            int val;
            if (!(cin >> val)) {
                if (cin.eof()) { cout << "\nInput ended. Exiting.\n"; exit(0); }
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Please enter a valid number.\n> ";
                continue;
            }
            if (val < lo || val > hi) {
                cout << "Enter a number between " << lo << " and " << hi << ".\n> ";
                continue;
            }
            return val;
        }
    }

    // Parses input like "e2e4" or "e7e8q" into board coordinates, then
    // matches it against the actual legal moves so illegal moves are rejected.
    Move readHumanMove(Board& board, PColor turn) {
        vector<Move> legal = board.legalMoves(turn);

        while (true) {
            cout << "Your move: ";
            string input;
            if (!(cin >> input)) {
                if (cin.eof()) { cout << "\nInput ended. Exiting.\n"; exit(0); }
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                continue;
            }

            if (input == "quit" || input == "exit") {
                cout << "Thanks for playing!\n";
                exit(0);
            }

            if (input.size() < 4) {
                cout << "Format should be like e2e4. Try again.\n";
                continue;
            }

            int fc = tolower(input[0]) - 'a';
            int fr = input[1] - '1';
            int tc = tolower(input[2]) - 'a';
            int tr = input[3] - '1';
            PieceType promo = PieceType::NONE;

            if (input.size() >= 5) {
                switch (tolower(input[4])) {
                    case 'q': promo = PieceType::QUEEN; break;
                    case 'r': promo = PieceType::ROOK; break;
                    case 'b': promo = PieceType::BISHOP; break;
                    case 'n': promo = PieceType::KNIGHT; break;
                    default: break;
                }
            }

            if (!Board::inBounds(fr, fc) || !Board::inBounds(tr, tc)) {
                cout << "Invalid square. Use letters a-h and numbers 1-8.\n";
                continue;
            }

            // Find a matching legal move (this enforces ALL chess rules —
            // check safety, piece movement patterns, castling conditions, etc.)
            for (auto& m : legal) {
                if (m.fr == fr && m.fc == fc && m.tr == tr && m.tc == tc) {
                    if (m.promotion != PieceType::NONE) {
                        if (m.promotion == promo || (promo == PieceType::NONE && m.promotion == PieceType::QUEEN)) {
                            return m;
                        }
                    } else {
                        return m;
                    }
                }
            }

            cout << Color::RED << "That's not a legal move. Try again.\n" << Color::RESET;
        }
    }

    void printBanner() {
        cout << Color::BOLD << Color::GREEN <<
R"(
   _____ _
  / ____| |
 | |    | |__   ___  ___ ___
 | |    | '_ \ / _ \/ __/ __|
 | |____| | | |  __/\__ \__ \
  \_____|_| |_|\___||___/___/
)"
        << Color::RESET
        << Color::GREY << "   Minimax + Alpha-Beta AI opponent\n\n" << Color::RESET;
    }
};

// ============================================================================
//  ENTRY POINT
// ============================================================================
int main() {
    Game game;
    game.run();
    return 0;
}
