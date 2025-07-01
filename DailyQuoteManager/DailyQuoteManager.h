#ifndef DAILYQUOTEMANAGER_H
#define DAILYQUOTEMANAGER_H

#include <QObject>
#include <QTextBrowser>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QGraphicsOpacityEffect>

class DailyQuoteManager : public QObject
{
    Q_OBJECT

public:
    explicit DailyQuoteManager(QTextBrowser *target,
                               QObject *parent = nullptr);
    QString currentQuote() const { return m_current; }

public slots:
    void refresh();

private slots:
    void fetchQuote();
    void onNetworkReply(QNetworkReply *reply);

private:
    void processQuoteResponse(const QByteArray &responseData);
    QString removeHtmlTags(const QString &input);

    QTextBrowser           *m_textBrowser;
    QTimer                  m_timer;
    QString                 m_current;
    QNetworkAccessManager   m_netManager;
};

#endif // DAILYQUOTEMANAGER_H
