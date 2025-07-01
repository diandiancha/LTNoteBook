#include "DailyQuoteManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QUrl>
#include <QPropertyAnimation>
#include <QDebug>

DailyQuoteManager::DailyQuoteManager(QTextBrowser *target, QObject *parent)
    : QObject(parent)
    , m_textBrowser(target)
{
    connect(&m_netManager, &QNetworkAccessManager::finished,
            this, &DailyQuoteManager::onNetworkReply);

    m_timer.setInterval(60 * 1000);
    connect(&m_timer, &QTimer::timeout, this, &DailyQuoteManager::fetchQuote);
    m_timer.start();

    fetchQuote();
}

void DailyQuoteManager::refresh()
{
    fetchQuote();
}

void DailyQuoteManager::fetchQuote()
{
    static const QUrl apiUrl("https://go-quote.azurewebsites.net/");
    QNetworkRequest request(apiUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Qt6-NoteBook");
    m_netManager.get(request);

    qDebug() << "[DailyQuoteManager] Fetching JSON quote from" << apiUrl.toString();
}

void DailyQuoteManager::onNetworkReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        m_textBrowser->setHtml(
            "<p style='color:red; font-style:italic;'>"
            "Failed to fetch quote.<br>"
            "Please check your network."
            "</p>"
            );
        qWarning() << "[DailyQuoteManager] Network error:" << reply->errorString();
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();
    processQuoteResponse(data);
}

void DailyQuoteManager::processQuoteResponse(const QByteArray &responseData)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        m_textBrowser->setHtml(
            "<p style='color:red; font-style:italic;'>"
            "Invalid response format."
            "</p>"
            );
        qWarning() << "[DailyQuoteManager] JSON parse error:" << err.errorString()
                   << " Raw data:" << responseData;
        return;
    }

    QJsonObject obj = doc.object();

    QString quote  = obj.value("text").toString().trimmed();
    QString author = obj.value("author").toString().trimmed();

    if (quote.isEmpty() || author.isEmpty()) {
        m_textBrowser->setHtml(
            "<p style='color:red; font-style:italic;'>"
            "Empty quote or author."
            "</p>"
            );
        qWarning() << "[DailyQuoteManager] Missing 'text' or 'author' in JSON:" << obj;
        return;
    }

    QString html = QString(R"(
  <div style="padding:10px 20px 10px 0;">
    <blockquote style="font-size:16px; color:#A6B0C3; margin:0 0 8px 0;">%1</blockquote>
    <p style="text-align:right; font-size:14px; color:#FFFFFF; margin:0;">— %2</p>
  </div>
)").arg(quote, author);
    m_textBrowser->setHtml(html);
    m_current = quote + " — " + author;

    auto *effect = qobject_cast<QGraphicsOpacityEffect*>(m_textBrowser->graphicsEffect());
    if (!effect) {
        effect = new QGraphicsOpacityEffect(m_textBrowser);
        m_textBrowser->setGraphicsEffect(effect);
    }
    effect->setOpacity(0.0);

    auto *anim = new QPropertyAnimation(effect, "opacity", m_textBrowser);
    anim->setDuration(200);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

QString DailyQuoteManager::removeHtmlTags(const QString &input)
{
    return input;
}
