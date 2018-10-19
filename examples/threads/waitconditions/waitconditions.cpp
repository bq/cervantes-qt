/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui>

#include <stdio.h>
#include <stdlib.h>

//! [0]
#ifdef Q_WS_S60
const int DataSize = 300;
#else
const int DataSize = 100000;
#endif

const int BufferSize = 8192;
char buffer[BufferSize];

QWaitCondition bufferNotEmpty;
QWaitCondition bufferNotFull;
QMutex mutex;
int numUsedBytes = 0;
//! [0]

//! [1]
class Producer : public QThread
//! [1] //! [2]
{
public:
    Producer(QObject *parent = NULL) : QThread(parent)
    {
    }

    void run()
    {
        qsrand(QTime(0,0,0).secsTo(QTime::currentTime()));

        for (int i = 0; i < DataSize; ++i) {
            mutex.lock();
            if (numUsedBytes == BufferSize)
                bufferNotFull.wait(&mutex);
            mutex.unlock();

            buffer[i % BufferSize] = "ACGT"[(int)qrand() % 4];

            mutex.lock();
            ++numUsedBytes;
            bufferNotEmpty.wakeAll();
            mutex.unlock();
        }
    }
};
//! [2]

//! [3]
class Consumer : public QThread
//! [3] //! [4]
{
    Q_OBJECT
public:
    Consumer(QObject *parent = NULL) : QThread(parent)
    {
    }

    void run()
    {
        for (int i = 0; i < DataSize; ++i) {
            mutex.lock();
            if (numUsedBytes == 0)
                bufferNotEmpty.wait(&mutex);
            mutex.unlock();

    #ifdef Q_WS_S60
            emit stringConsumed(QString(buffer[i % BufferSize]));
    #else
            fprintf(stderr, "%c", buffer[i % BufferSize]);
    #endif

            mutex.lock();
            --numUsedBytes;
            bufferNotFull.wakeAll();
            mutex.unlock();
        }
        fprintf(stderr, "\n");
    }

signals:
    void stringConsumed(const QString &text);
};
//! [4]

#ifdef Q_WS_S60
class PlainTextEdit : public QPlainTextEdit
{
    Q_OBJECT
public:
    PlainTextEdit(QWidget *parent = NULL) : QPlainTextEdit(parent), producer(NULL), consumer(NULL)
    {
        setTextInteractionFlags(Qt::NoTextInteraction);

        producer = new Producer(this);
        consumer = new Consumer(this);

        QObject::connect(consumer, SIGNAL(stringConsumed(const QString &)), SLOT(insertPlainText(const QString &)), Qt::BlockingQueuedConnection);

        QTimer::singleShot(0, this, SLOT(startThreads()));
    }

protected:
    Producer *producer;
    Consumer *consumer;

protected slots:
    void startThreads()
    {
        producer->start();
        consumer->start();
    }
};
#endif

//! [5]
int main(int argc, char *argv[])
//! [5] //! [6]
{
#ifdef Q_WS_S60
    QApplication app(argc, argv);

    PlainTextEdit console;
    console.showMaximized();

    return app.exec();
#else
    QCoreApplication app(argc, argv);
    Producer producer;
    Consumer consumer;
    producer.start();
    consumer.start();
    producer.wait();
    consumer.wait();
    return 0;
#endif
}
//! [6]

#include "waitconditions.moc"
