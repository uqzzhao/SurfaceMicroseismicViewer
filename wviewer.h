#ifndef WVIEWER_H
#define WVIEWER_H

#include <QMainWindow>
#include <QFileInfoList>
#include <QTreeWidgetItem>
#include <QVector>
#include <QFileInfoList>
#include <QDoubleValidator>

#include "qwt_plot_curve.h"
#include "Zoomer.h"
#include "csegy.h"

namespace Ui {
class WViewer;
}

struct Geophone
{
    QString name;
    double x;
    double y;
    double z;
    double distance;
};

class WViewer : public QMainWindow
{
    Q_OBJECT

public:
    explicit WViewer(QWidget *parent = 0);
    ~WViewer();

private:
    Ui::WViewer *ui;


private:
    QString m_strDir;
    QString m_strCurrentFile;

    QFileInfoList  m_folderList;

    int m_nGeoCount;
    int m_nTraceLen;
    int m_nTotalTraceNum;

    int m_nComponent;

    float m_nScale;
    double absMax[100];

    int m_nSequence; //0为检波器不排序， 1为排序

private:
    CSegy m_segy;

    QVector<QwtPlotCurve *> m_Curve; //图像曲线
    QVector<QVector<double>*> m_Traces; //一个SEGY文件中所有道的数据
    QVector<QFileInfoList> m_vFileInfoList; //存储各目录下的文件名列表


    Zoomer *m_pZoomer;
    QString m_szCurrentFile;

    QList<Geophone*> m_Geophone; //检波器信息

    //震源位置
    QDoubleValidator *m_sourceXValidator, *m_sourceYValidator, *m_sourceZValidator;
    double m_sourceX, m_sourceY, m_sourceZ;





public:

    void setParameters();
    void getSource();
    void calDistance();
    void sortGeophones();
    void readData(const char *szFileName);
    void pseudoTrace();
    void LoadData(int sequence, QString szFileName);
    void ReadGeophones(QString strPath, QList<Geophone *> &geophone);
    void clearData(QVector<QVector<double> *> &v);
    void clearCurve(QVector<QwtPlotCurve *> &v);
    void Plot(int geoNum, int component);
    void setGeophones(int geoNum, QVector<QwtPlotCurve *> &v);

    QFileInfoList scanDirFile(QTreeWidgetItem *root, QString path, QVector<QFileInfoList> &vectorList);

    QString getCurrentFileName(QTreeWidgetItem *itemfile);




private slots:
    void OnSelectDir();
    void OnLoadTriggered();
    void OnAboutTriggered();
    void OnSelectNewFile(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void update();
    void updateWaveform();
    void OnXSelected();
    void OnYSelected();
    void OnZSelected();
    void On3cSelected();
    void OnSortChecked();



};

#endif // WVIEWER_H
