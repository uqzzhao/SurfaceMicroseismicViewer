#include "wviewer.h"
#include "ui_wviewer.h"

#include <QDirIterator>
#include <QFileDialog>
#include <QMessageBox>
#include <math.h>

#include "qwt_plot_grid.h"

#include "util.h"

WViewer::WViewer(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::WViewer)
{
    ui->setupUi(this);

    connect(ui->actionSelectDirectory, SIGNAL(triggered(bool)), this, SLOT(OnSelectDir()));
    connect(ui->actionLoad, SIGNAL(triggered(bool)), this, SLOT(OnLoadTriggered()));
    connect(ui->actionAbout, SIGNAL(triggered(bool)), this, SLOT(OnAboutTriggered()));
    connect(ui->treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(OnSelectNewFile(QTreeWidgetItem*,QTreeWidgetItem*)));
    connect(ui->rbt3C, SIGNAL(clicked(bool)), this, SLOT(On3cSelected()));
    connect(ui->rbtX,  SIGNAL(clicked(bool)), this, SLOT(OnXSelected()));
    connect(ui->rbtY,  SIGNAL(clicked(bool)), this, SLOT(OnYSelected()));
    connect(ui->rbtZ,  SIGNAL(clicked(bool)), this, SLOT(OnZSelected()));
    connect(ui->cbxSort, SIGNAL(stateChanged(int)), this, SLOT(OnSortChecked()));

    ui->rbtZ->setChecked(true);
    ui->Plot3C->setTitle("Z-Component Waveform");
    ui->rbtX->setEnabled(false);
    ui->rbtY->setEnabled(false);
    ui->rbtZ->setEnabled(false);
    ui->rbt3C->setEnabled(false);

    ui->cbxSort->setEnabled(false);
    ui->edtX->setEnabled(false);
    ui->edtY->setEnabled(false);
    ui->edtZ->setEnabled(false);

    m_sourceXValidator=new QDoubleValidator(0.0,0.0,0.0,this);
    m_sourceYValidator=new QDoubleValidator(0.0,0.0,0.0,this);
    m_sourceZValidator=new QDoubleValidator(0.0,0.0,0.0,this);
    ui->edtX->setValidator(m_sourceXValidator);
    ui->edtY->setValidator(m_sourceYValidator);
    ui->edtZ->setValidator(m_sourceZValidator);


    //-------------------------微震三分量数据绘图

    ui->Plot3C->enableAxis(QwtPlot::yLeft);
    ui->Plot3C->enableAxis(QwtPlot::yRight, false);
    ui->Plot3C->enableAxis(QwtPlot::xTop, false);
    ui->Plot3C->enableAxis(QwtPlot::xBottom);

    ui->Plot3C->setAxisTitle(ui->Plot3C->yLeft, tr("Geophones"));
    ui->Plot3C->setAxisScale(ui->Plot3C->yLeft, 0,13);
    ui->Plot3C->setAxisTitle(ui->Plot3C->xBottom, tr("Samples"));
    ui->Plot3C->setAxisScale(ui->Plot3C->xBottom, 0,1000);

    ui->Plot3C->setLineWidth(1);
    ui->Plot3C->setFrameStyle(QFrame::Box| QFrame::Plain);


    ui->Plot3C->setAutoDelete(true);

    m_nGeoCount =10;
    m_nTotalTraceNum =3;
    m_nTraceLen =1000;
    m_nScale =1.0;
    m_nComponent =0;
    m_nSequence =0;

    //absMax[100]={0};


}

WViewer::~WViewer()
{
    delete ui;
}

void WViewer::setParameters()
{

    QDir dir(m_strDir);          //遍历各级子目录
    m_folderList = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);   //获取当前所有目录
    m_nGeoCount =m_folderList.size(); //根据遍历的文件夹个数确定检波器个数

    QString str = m_vFileInfoList[1].at(1).absoluteFilePath();

    char absFilePath[200];
    memset((char*)&absFilePath,0,200);
    QStringData *data = str.data_ptr();
    ushort *s = data->data();

    for (int i = 0; i<data->size; i++ )
    {
        absFilePath[i] = s[i];
    }

    if(!m_segy.OpenFile(absFilePath))
    {
        QMessageBox::information(this, tr("Open SEGY File"), tr("SEGY file open failed!"));
        return;
    }



    m_nTotalTraceNum=m_segy.getTotalTraceNumber();
    m_nTraceLen = m_segy.getSamplesNumber();
    m_segy.closeFile();


}

void WViewer::getSource()
{

    QString str;

    m_sourceX=ui->edtX->text().toDouble();
    if(m_sourceX<0.0)
    {
        m_sourceX=0.0;
        ui->edtX->setText(str.sprintf("%.1f",m_sourceX));
    }

    m_sourceY=ui->edtY->text().toDouble();
    if(m_sourceY<0.0)
    {
        m_sourceY=0.0;
        ui->edtY->setText(str.sprintf("%.1f",m_sourceY));
    }

    m_sourceZ=ui->edtZ->text().toDouble();
    if(m_sourceZ<0.0)
    {
        m_sourceZ=0.0;
        ui->edtZ->setText(str.sprintf("%.1f",m_sourceZ));
    }

}

void WViewer::calDistance()
{
    getSource();

    for (int i=0; i<m_Geophone.count(); i++)
    {
        double dis = sqrt((m_Geophone[i]->x - m_sourceX)*(m_Geophone[i]->x - m_sourceX)
                          +(m_Geophone[i]->y - m_sourceY)*(m_Geophone[i]->y - m_sourceY)
                          +(m_Geophone[i]->z - m_sourceZ)*(m_Geophone[i]->z - m_sourceZ));
        m_Geophone[i]->distance = dis;
    }


}

void WViewer::sortGeophones()
{

    for (int i=0; i<m_Geophone.count()-1; ++i)
    {
        for (int j=0; j<m_Geophone.count()-1-i; ++j)
        {
            if (m_Geophone[j]->distance>m_Geophone[j+1]->distance)
            {
                Geophone *tmp = m_Geophone[j];
                m_Geophone.replace(j, m_Geophone[j+1]);
                m_Geophone.replace(j+1, tmp);

            }

        }
    }


}

void WViewer::readData(const char *szFileName)
{
    m_segy.OpenFile(szFileName);

    double sampleValue= 0.0;


    for (int i=0; i<m_nTotalTraceNum; i++)
    {

        QVector<double>* m_ChangeData=new QVector<double>;
        m_ChangeData->clear();

    float *pTrace = m_segy.GetTraceData(i+1);

        QVector <double> tempVector;
        tempVector.clear();

        for (int j=0; j<m_nTraceLen; j++)
        {
            m_ChangeData->append(sampleValue);

        }


        for (int k=0; k<m_nTraceLen; k++)
        {
            tempVector<<(double)pTrace[k];

        }
        //地震道单道归一化，每个采样点振幅值除以该道最大振幅值的绝对值
        double absMin=0;
        GetVectorMax(tempVector, absMax[i]);

        for (int k=0; k<m_nTraceLen; k++)
        {

            tempVector[k] = ( tempVector[k]-absMin)/(absMax[i]-absMin);

            m_ChangeData->replace (k, tempVector[k]);

        }

        m_Traces<<m_ChangeData;
      delete []pTrace;
    }

    m_segy.closeFile();
}

void WViewer::pseudoTrace()
{

    double sampleValue= 0.0;


    for (int i=0; i<m_nTotalTraceNum; i++)
    {

        QVector<double>* m_ChangeData = new QVector<double>;
        m_ChangeData->clear();


        for (int k=0; k<m_nTraceLen; k++)
        {


            m_ChangeData->append(sampleValue);

        }

        m_Traces<<m_ChangeData;

    }
}

void WViewer::LoadData(int sequence, QString szFileName)
{
    clearData(m_Traces);
//    while(!m_Traces.isEmpty())
//        delete m_Traces.takeFirst();

    switch (sequence) {
    case 0:


        for(int i = 0; i < m_nGeoCount; i++)
        {


            int totalCount = m_vFileInfoList[i].count();
            for (int j=0; j<totalCount; j++)
            {

                int   count = j+1;

                if (m_vFileInfoList[i].at(j).absoluteFilePath().right(10) ==  szFileName.right(10))
                {
                    QString str = m_vFileInfoList[i].at(j).absoluteFilePath();

                    char absFilePath[200];
                    qString2ConstChar(str, absFilePath);
                    readData(absFilePath);
                    break;    //一定要加上这个break, 读进来数据就应结束循环，break为跳出

                }

                else if (totalCount - count  == 0)
                    pseudoTrace();
            }

        }

        break;
    case 1:

        calDistance();
        sortGeophones();

        for(int i = 0; i < m_nGeoCount; i++)
        {

            for( int k = 0; k < m_nGeoCount; k++)
            {
                QString tempStr = m_vFileInfoList[k].at(0).absoluteFilePath().right(23) ;
                if( tempStr.left(5) == m_Geophone[i]->name)
                {
                    int totalCount = m_vFileInfoList[k].count();
                    for (int j=0; j<totalCount; j++)
                    {

                        int   count = j+1;

                        if (m_vFileInfoList[k].at(j).absoluteFilePath().right(10) ==  szFileName.right(10))
                        {
                            QString str = m_vFileInfoList[k].at(j).absoluteFilePath();

                            char absFilePath[200];
                            qString2ConstChar(str, absFilePath);
                            readData(absFilePath);
                            break;    //一定要加上这个break, 读进来数据就应结束循环，break为跳出

                        }

                        else if (totalCount - count  == 0)
                            pseudoTrace();



                    }
                    break;

                }
            }
        }
        break;

    default:
        break;
    }


}

void WViewer::ReadGeophones(QString strPath, QList<Geophone*> &geophone)
{
    Geophone *geo;

    QFile file(strPath);

    if(!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::information(this,QStringLiteral("File open failed!"),file.errorString());
        return ;
    }


    //获取数据
    QStringList list;
    list.clear();
    QTextStream in(&file);


    while(!in.atEnd())
    {
        QString fileLine = in.readLine();
        list = fileLine.split(" ", QString::SkipEmptyParts);

        geo = new Geophone;
        geo->name = list[0];
        geo->x = list[1].toDouble();
        geo->y = list[2].toDouble();
        geo->z = list[3].toDouble();
        geo->distance = list[4].toDouble();
        geophone<<geo;

    }



    file.close();

}
void WViewer::clearData(QVector<QVector<double> *> &v)
{
    for (int i=0;i<v.count();i++)
    {

        delete v[i];
    }
    v.clear();
}

void WViewer::clearCurve(QVector<QwtPlotCurve *> &v)
{
    for (int i=0;i<v.count();i++)
    {
        v[i]->detach();
        delete v[i];
    }
    v.clear();
}

void WViewer::Plot(int geoNum, int component)
{


    QVector<QwtPlotCurve*> curves;
    clearCurve(curves);
    setGeophones(m_nGeoCount, curves);

    int i,k;
    double j=0.0;
    double m_TempValue;
    QVector<double> m_TempData;
    QVector<double> vectorX3c;

    for(i=0;i<curves.count();i++)
        curves[i]->setVisible(false);


    vectorX3c.clear();

    for (int i=0; i<m_nTraceLen; i++)
    {
        vectorX3c<<j;
        j+=1.0;

    }



    for(i=0;i<geoNum*3;i++)
    {
        m_TempData.clear();
        if(i%3==component && component!=3)
        {
            for(k=0;k<m_Traces[i]->count();k++)
            {
                m_TempValue=(*m_Traces[i])[k];
                m_TempValue=m_nScale*0.5*m_TempValue;  //显示波形的幅度
                m_TempValue=1+i/3+m_TempValue;
                m_TempData<<m_TempValue;
            }

            curves[i]->setSamples(vectorX3c,m_TempData);
            curves[i]->setVisible(true);
        }
        else if(component==3)  //XYZ三条曲线一同显示
        {
            for(k=0;k<m_Traces[i]->count();k++)
            {
                m_TempValue=(*m_Traces[i])[k];
                m_TempValue=m_nScale*0.5*m_TempValue;  //显示波形的幅度
                m_TempValue=1+i/3+m_TempValue;
                m_TempData<<m_TempValue;
            }
            curves[i]->setSamples(vectorX3c,m_TempData);
            curves[i]->setVisible(true);
        }
    }

    //加入网格
    QwtPlotGrid *grid=new QwtPlotGrid();
    grid->enableXMin(true);
    grid->setMajorPen(QPen(Qt::lightGray,0,Qt::DotLine));
    grid->setMinorPen(QPen(Qt::lightGray,0,Qt::DotLine));
    grid->attach(ui->Plot3C);




    //加入放大缩小所选择矩形区域
    QRectF rect(0,0,m_nTraceLen,m_nGeoCount+1);

    //设置放大缩小zoomer功能
    QwtPlotCanvas* traceCanvas = new QwtPlotCanvas();
    ui->Plot3C->setCanvas(traceCanvas);
    m_pZoomer = new Zoomer(traceCanvas,rect);

    m_pZoomer->setRect(rect);
    m_pZoomer->setZoomBase(rect);
    m_pZoomer->setMousePattern(QwtEventPattern::MouseSelect2,
                               Qt::RightButton,Qt::ControlModifier);
    m_pZoomer->setMousePattern(QwtEventPattern::MouseSelect3,
                               Qt::RightButton);
    m_pZoomer->setRubberBandPen(QPen(Qt::red, 2, Qt::DotLine));
    m_pZoomer->setTrackerMode(QwtPicker::AlwaysOff);
    m_pZoomer->setEnabled(true);



    ui->Plot3C->replot();
    //ui->Plot3C->removeItem();



}

void WViewer::setGeophones(int geoNum, QVector<QwtPlotCurve *> &v)
{

    int i;

    //    while(!v.isEmpty())
    //        delete v.takeFirst();


    ui->Plot3C->setAxisScale(ui->Plot3C->yLeft,0,(double)(geoNum+1));
    ui->Plot3C->setAxisScale(ui->Plot3C->xBottom, 0, (double)m_nTraceLen);

    QPen greenPen;
    //greenPen.setColor(QColor(0,100,0)); //深绿色
    greenPen.setColor(QColor(34,139,34));  //森林绿


    for(i=0;i<geoNum*3;i++)
    {
        QwtPlotCurve* NewCurve=new QwtPlotCurve;
        NewCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        if(i%3==0)
            NewCurve->setPen(greenPen);
        else if(i%3==1)
            NewCurve->setPen(QPen(Qt::red));
        else if(i%3==2)
            NewCurve->setPen(QPen(Qt::blue));


        NewCurve->attach(ui->Plot3C);
        NewCurve->setVisible(false);
        v<<NewCurve;
    }

    //ui->Plot3C->replot();


}

QFileInfoList WViewer::scanDirFile(QTreeWidgetItem *root, QString path, QVector<QFileInfoList> &vectorList) //参数为主函数中添加的item和路径名
{
    /*添加path路径文件*/
    QDir dir(path);          //遍历各级子目录
    QDir dir_file(path);    //遍历子目录中所有文件
    dir_file.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);        //获取当前所有文件
    dir_file.setSorting(QDir::Size | QDir::Reversed);
    QFileInfoList list_file = dir_file.entryInfoList();
    for (int i = 0; i < list_file.size(); ++i) {       //将当前目录中所有文件添加到treewidget中
        QFileInfo fileInfo = list_file.at(i);
        QString name2=fileInfo.fileName();
        QTreeWidgetItem* child = new QTreeWidgetItem(QStringList()<<name2);
        child->setIcon(0, QIcon(":/file/image/link.ico"));
        child->setCheckState(1, Qt::Checked);
        root->addChild(child);
    }


    QFileInfoList file_list=dir.entryInfoList(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    QFileInfoList folder_list = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);   //获取当前所有目录

    for(int i = 0; i != folder_list.size(); i++)         //自动递归添加各目录到上一级目录
    {

        QString namepath = folder_list.at(i).absoluteFilePath();    //获取路径
        QFileInfo folderinfo= folder_list.at(i);
        QString name=folderinfo.fileName();      //获取目录名
        QTreeWidgetItem* childroot = new QTreeWidgetItem(QStringList()<<name);
        childroot->setIcon(0, QIcon(":/file/image/link.ico"));
        childroot->setCheckState(1, Qt::Checked);
        root->addChild(childroot);              //将当前目录添加成path的子项
        QFileInfoList child_file_list = scanDirFile(childroot,namepath,  m_vFileInfoList);          //进行递归
        file_list.append(child_file_list);
        file_list.append(name);


    }
    m_vFileInfoList<<file_list;

    return file_list;
}

QString WViewer::getCurrentFileName(QTreeWidgetItem *itemfile)
{

    QStringList filepath;
    while(itemfile!=NULL)

    {

        filepath<<itemfile->text(0); //获取itemfile名称

        itemfile=itemfile->parent(); //将itemfile指向父item

    }

    QString strpath;

    //cout<<filepath.size()<<endl;

    for(int i=(filepath.size()-1);i>=0;i--) //QStringlist类filepath反向存着初始item的路径

    { //将filepath反向输出，相应的加入’/‘

        strpath+=filepath.at(i);

        if(i!=0)

            strpath+="/";

    }


    return strpath;
}

void WViewer::OnSelectDir()
{

    m_strDir=QFileDialog::getExistingDirectory(this,tr("Select Directory"),tr("Project directory"),
                                               QFileDialog::DontResolveSymlinks);
    if(m_strDir.length()==0)
        return;
    if(m_strDir.right(1)!=tr("/"))
        m_strDir=m_strDir+tr("/");

    if (m_strDir.length()>0 ) //判断路径是否存在
    {

        QTreeWidgetItem* root = new QTreeWidgetItem(QStringList()<<m_strDir);
        root->setCheckState(1, Qt::Checked);
        ui->treeWidget->addTopLevelItem(root);
        scanDirFile(root, m_strDir, m_vFileInfoList); //遍地添加/home/XXX目录下所有文件，此函数具体内容如下
        setParameters();


    }




}

void WViewer::OnLoadTriggered()
{
    QString strFile=QFileDialog::getOpenFileName(this, tr("Open File"),  tr(""),

                                                 tr("Files (*.prn)"));
    if(strFile.length()==0)
        return;


    if (strFile.length()>0 ) //判断路径是否存在
    {

        ReadGeophones(strFile, m_Geophone);

        ui->cbxSort->setEnabled(true);
        ui->edtX->setEnabled(true);
        ui->edtY->setEnabled(true);
        ui->edtZ->setEnabled(true);

    }
}

void WViewer::OnAboutTriggered()
{

    QString messages = "DeepListen Surface Microseismic Viewer\n\nAuthor：Zhengguang Zhao\nEmail：  zzhao@deep-listen.com \n\
            \n\
            \nSurface Microseismic Viewer, a microseismic waveform display module from\nDeepListen, a downhole/surface microseismic data processing software.\n\
            \n(C) 2018-2019 DeepListen Pty Ltd, Australia\n\
            \n\
            \nFor more information about Surface Microseismic Viewer, please visit us at:\n\
            \n-------------  http://www.deep-listen.com  ------------\n";

    QMessageBox::information(this, tr("About DeepListen"), messages);



}

void WViewer::OnSelectNewFile(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    QStringList filepath;

    QTreeWidgetItem *item=current; //获取被点击的item
    m_szCurrentFile = getCurrentFileName(item);

    if (m_szCurrentFile.right(4) != ".sgy" && m_szCurrentFile.right(5) != ".segy")
    {
        QMessageBox::information(this, tr("Tips!"), tr("Please reselect a SEGY file!"));
        return;
    }


    update();


    ui->rbtX->setEnabled(true);
    ui->rbtY->setEnabled(true);
    ui->rbtZ->setEnabled(true);
    ui->rbt3C->setEnabled(true);


}

void WViewer::update()
{


    LoadData(m_nSequence, m_szCurrentFile);

    updateWaveform();
}

void WViewer::updateWaveform()
{


    ui->Plot3C->detachItems(QwtPlotItem::Rtti_PlotItem, true);
    //ui->Plot3C->replot();

    Plot(m_nGeoCount, m_nComponent);
}


void WViewer::OnXSelected()
{
    m_nComponent = 1;
    ui->Plot3C->setTitle("X-Component Waveform");
    updateWaveform();


}

void WViewer::OnYSelected()
{
    m_nComponent = 2;
    ui->Plot3C->setTitle("Y-Component Waveform");
    updateWaveform();

}


void WViewer::OnZSelected()
{
    m_nComponent =0;
    ui->Plot3C->setTitle("Z-Component Waveform");
    updateWaveform();

}


void WViewer::On3cSelected()
{
    m_nComponent = 3;
    ui->Plot3C->setTitle("3 Components Waveform");
    updateWaveform();

}

void WViewer::OnSortChecked()
{
    if (ui->cbxSort->checkState() == Qt::Checked)
    {
        m_nSequence = 1;

        getSource();

        update();

    }
    else
    {
        m_nSequence = 0;

        update();

    }
}
