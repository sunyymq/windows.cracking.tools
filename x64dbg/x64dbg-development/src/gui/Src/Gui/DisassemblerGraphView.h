#ifndef DISASSEMBLERGRAPHVIEW_H
#define DISASSEMBLERGRAPHVIEW_H

#include <QObject>
#include <QWidget>
#include <QAbstractScrollArea>
#include <QPaintEvent>
#include <QTimer>
#include <QSize>
#include <QResizeEvent>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <algorithm>
#include <QMutex>
#include "Bridge.h"
#include "RichTextPainter.h"
#include "QBeaEngine.h"
#include "ActionHelpers.h"

class MenuBuilder;
class CachedFontMetrics;
class GotoDialog;
class XrefBrowseDialog;

class DisassemblerGraphView : public QAbstractScrollArea, public ActionHelper<DisassemblerGraphView>
{
    Q_OBJECT
public:
    struct DisassemblerBlock;

    struct Point
    {
        int row; //point[0]
        int col; //point[1]
        int index; //point[2]
    };

    struct DisassemblerEdge
    {
        QColor color;
        DisassemblerBlock* dest;
        std::vector<Point> points;
        int start_index = 0;

        QPolygonF polyline;
        QPolygonF arrow;

        void addPoint(int row, int col, int index = 0)
        {
            Point point = {row, col, 0};
            this->points.push_back(point);
            if(int(this->points.size()) > 1)
                this->points[this->points.size() - 2].index = index;
        }
    };

    struct Token
    {
        int start; //token[0]
        int length; //token[1]
        QString type; //token[2]
        duint addr; //token[3]
        QString name; //token[4]
    };

    struct HighlightToken
    {
        QString type; //highlight_token[0]
        duint addr; //highlight_token[1]
        QString name; //highlight_token[2]

        bool equalsToken(const Token & token)
        {
            return this->type == token.type &&
                   this->addr == token.addr &&
                   this->name == token.name;
        }

        static HighlightToken* fromToken(const Token & token)
        {
            //TODO: memory leaks
            auto result = new HighlightToken();
            result->type = token.type;
            result->addr = token.addr;
            result->name = token.name;
            return result;
        }
    };

    struct Text
    {
        std::vector<RichTextPainter::List> lines;

        Text() {}

        Text(const QString & text, QColor color, QColor background)
        {
            RichTextPainter::List richText;
            RichTextPainter::CustomRichText_t rt;
            rt.highlight = false;
            rt.text = text;
            rt.textColor = color;
            rt.textBackground = background;
            rt.flags = rt.textBackground.alpha() ? RichTextPainter::FlagAll : RichTextPainter::FlagColor;
            richText.push_back(rt);
            lines.push_back(richText);
        }

        Text(const RichTextPainter::List & richText)
        {
            lines.push_back(richText);
        }

        QString ToQString() const
        {
            QString result;
            for(auto & line : lines)
            {
                for(auto & t : line)
                {
                    result += t.text;
                }
            }
            return std::move(result);
        }
    };

    struct Instr
    {
        duint addr = 0;
        Text text;
        std::vector<unsigned char> opcode; //instruction bytes
    };

    struct Block
    {
        Text header_text;
        std::vector<Instr> instrs;
        std::vector<duint> exits;
        duint entry = 0;
        duint true_path = 0;
        duint false_path = 0;
        bool terminal = false;
        bool indirectcall = false;
    };

    struct DisassemblerBlock
    {
        DisassemblerBlock() {}
        explicit DisassemblerBlock(Block & block)
            : block(block) {}

        Block block;
        std::vector<DisassemblerEdge> edges;
        std::vector<duint> incoming;
        std::vector<duint> new_exits;

        qreal x = 0.0;
        qreal y = 0.0;
        int width = 0;
        int height = 0;
        int col = 0;
        int col_count = 0;
        int row = 0;
        int row_count = 0;
    };

    struct Function
    {
        bool ready;
        duint entry;
        duint update_id;
        std::vector<Block> blocks;
    };

    struct Analysis
    {
        duint entry = 0;
        std::unordered_map<duint, Function> functions;
        bool ready = false;
        duint update_id = 0;
        QString status = "Analyzing...";

        bool find_instr(duint addr, duint & func, duint & instr)
        {
            //TODO implement
            Q_UNUSED(addr);
            Q_UNUSED(func);
            Q_UNUSED(instr);
            return false;
        }

        //dummy class
    };

    enum class LayoutType
    {
        Wide,
        Medium,
        Narrow,
    };

    DisassemblerGraphView(QWidget* parent = nullptr);
    ~DisassemblerGraphView();
    void initFont();
    void adjustSize(int width, int height);
    void resizeEvent(QResizeEvent* event);
    duint get_cursor_pos();
    void set_cursor_pos(duint addr);
    std::tuple<duint, duint> get_selection_range();
    void set_selection_range(std::tuple<duint, duint> range);
    void copy_address();
    //void analysis_thread_proc();
    //void closeRequest();
    void paintNormal(QPainter & p, QRect & viewportRect, int xofs, int yofs);
    void paintOverview(QPainter & p, QRect & viewportRect, int xofs, int yofs);
    void paintEvent(QPaintEvent* event);
    bool isMouseEventInBlock(QMouseEvent* event);
    duint getInstrForMouseEvent(QMouseEvent* event);
    bool getTokenForMouseEvent(QMouseEvent* event, Token & token);
    bool find_instr(duint addr, Instr & instr);
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void mouseDoubleClickEvent(QMouseEvent* event);
    void prepareGraphNode(DisassemblerBlock & block);
    void adjustGraphLayout(DisassemblerBlock & block, int col, int row);
    void computeGraphLayout(DisassemblerBlock & block);
    void setupContextMenu();
    void keyPressEvent(QKeyEvent* event);

    template<typename T>
    using Matrix = std::vector<std::vector<T>>;
    using EdgesVector = Matrix<std::vector<bool>>;
    bool isEdgeMarked(EdgesVector & edges, int row, int col, int index);
    void markEdge(EdgesVector & edges, int row, int col, int index, bool used = true);
    int findHorizEdgeIndex(EdgesVector & edges, int row, int min_col, int max_col);
    int findVertEdgeIndex(EdgesVector & edges, int col, int min_row, int max_row);
    DisassemblerEdge routeEdge(EdgesVector & horiz_edges, EdgesVector & vert_edges, Matrix<bool> & edge_valid, DisassemblerBlock & start, DisassemblerBlock & end, QColor color);
    void renderFunction(Function & func);
    void show_cur_instr(bool force = false);
    bool navigate(duint addr);
    void fontChanged();
    void setGraphLayout(LayoutType layout);

signals:
    void displaySnowmanWidget();

public slots:
    void updateTimerEvent();
    void loadGraphSlot(BridgeCFGraphList* graph, duint addr);
    void graphAtSlot(duint addr);
    void updateGraphSlot();
    void followDisassemblerSlot();
    void colorsUpdatedSlot();
    void fontsUpdatedSlot();
    void shortcutsUpdatedSlot();
    void toggleOverviewSlot();
    void toggleSummarySlot();
    void selectionGetSlot(SELECTIONDATA* selection);
    void tokenizerConfigUpdatedSlot();
    void loadCurrentGraph();
    void disassembleAtSlot(dsint va, dsint cip);
    void gotoExpressionSlot();
    void gotoOriginSlot();
    void toggleSyncOriginSlot();
    void refreshSlot();
    void saveImageSlot();
    void setCommentSlot();
    void setLabelSlot();
    void xrefSlot();
    void decompileSlot();

private:
    QString status;
    Analysis analysis;
    duint function;
    QTimer* updateTimer;
    int baseline;
    qreal charWidth;
    int charHeight;
    int charOffset;
    int width;
    int height;
    int renderWidth;
    int renderHeight;
    int renderXOfs;
    int renderYOfs;
    duint cur_instr;
    int scroll_base_x;
    int scroll_base_y;
    duint update_id;
    bool scroll_mode;
    bool ready;
    int* desired_pos;
    std::unordered_map<duint, DisassemblerBlock> blocks;
    HighlightToken* highlight_token;
    std::vector<int> col_edge_x;
    std::vector<int> row_edge_y;
    CachedFontMetrics* mFontMetrics;
    MenuBuilder* mMenuBuilder;
    bool drawOverview;
    bool onlySummary;
    bool syncOrigin;
    int overviewXOfs;
    int overviewYOfs;
    qreal overviewScale;
    duint mCip;
    bool forceCenter;
    bool saveGraph;
    LayoutType layoutType;

    QAction* mToggleOverview;
    QAction* mToggleSummary;
    QAction* mToggleSyncOrigin;

    QColor disassemblyBackgroundColor;
    QColor disassemblySelectionColor;
    QColor disassemblyTracedColor;
    QColor disassemblyTracedSelectionColor;
    QColor jmpColor;
    QColor brtrueColor;
    QColor brfalseColor;
    QColor retShadowColor;
    QColor indirectcallShadowColor;
    QColor backgroundColor;
    QColor mAutoCommentColor;
    QColor mAutoCommentBackgroundColor;
    QColor mCommentColor;
    QColor mCommentBackgroundColor;
    QColor mLabelColor;
    QColor mLabelBackgroundColor;
    QColor mCipColor;
    QColor mCipBackgroundColor;
    QColor graphNodeColor;
    QColor mAddressColor;
    QColor mAddressBackgroundColor;

    BridgeCFGraph currentGraph;
    std::unordered_map<duint, duint> currentBlockMap;
    QBeaEngine disasm;
    GotoDialog* mGoto;
    XrefBrowseDialog* mXrefDlg;
};

#endif // DISASSEMBLERGRAPHVIEW_H
