#ifndef CR3WIDGET_H
#define CR3WIDGET_H

#include <qwidget.h>
#include <QScrollBar>

class LVDocView;

class CR3View : public QWidget
{
        Q_OBJECT

        Q_PROPERTY( QScrollBar* scrollBar READ scrollBar WRITE setScrollBar )

    public:
        CR3View( QWidget *parent = 0 );
        virtual ~CR3View();

        bool loadDocument( QString fileName );

        QScrollBar * scrollBar() const;

    public slots:
        void setScrollBar( QScrollBar * scroll );
        /// on scroll
        void scrollTo( int value );


    signals:
        //void fileNameChanged( const QString & );

    protected:
        virtual void paintEvent ( QPaintEvent * event );
        virtual void resizeEvent ( QResizeEvent * event );

        void updateScroll();

    private slots:

    private:
        LVDocView * _docview;
        QScrollBar * _scroll;

};

#endif // CR3WIDGET_H
