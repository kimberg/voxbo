

class VBHelp : public QWidget {
  Q_OBJECT
public:
  QTView(QWidget *parent=0,const char *name=0);
public slots:
  int NewSearch();
  int Close();
protected:
private slots:
private:
  QTextBrowser *browser;
  vector<string> sourcelist;
};
