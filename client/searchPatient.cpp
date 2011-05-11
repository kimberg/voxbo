/****************************************************************************
 ** 
 ** This interface is written to search patient information.
 ** 
 ** Because the interface has to be scrollable, QT designer is not used.
 **
 ****************************************************************************/

using namespace std;

#include <QtGui>
#include "searchPatient.h"
#include "dbclient.h"
#include "dbview.h"

// Ctor that builds search interface
SearchPatient::SearchPatient(DBclient *c)
  : QMainWindow()
{
  dbcp=c;
  mainArea = new QScrollArea;
  sp_ui = new QWidget; 

  setupUI();
  mainArea->setWidget(sp_ui);
  mainArea->setMinimumWidth(950);
  setCentralWidget(mainArea);
  setWindowTitle(tr("Search Patient"));

  setDockWindow();

  //setMinimumSize(400, 400);
  //resize(950, 400);
}

/* Set up new patient information interface. */
void SearchPatient::setupUI()
{
  spLayout = new QVBoxLayout;

  addTitle();    // add a simple title
  addSearch();   // add search criteria
  addButtons();  // add three pushbuttons at the end

  sp_ui->setLayout(spLayout);
}

/* Add the title on new patient interface. */
void SearchPatient::addTitle()
{
  QLabel *title = new QLabel("<B><center>Search Patient</center></B>");
  //title->setGeometry(200, 20, 100, 50);
  spLayout->addWidget(title);
  //spLayout->addSpacing(10);
}

/* Set up search interface. */
void SearchPatient::addSearch()
{
  s_field = new QComboBox;
  s_field->addItem("Any Field");

  searchScoreIDs.clear();
  map<string, DBscorename>::const_iterator iter;
  for (iter = dbcp->dbs.scorenames.begin(); iter != dbcp->dbs.scorenames.end(); ++iter) {
    if ((iter->second).flags.count("searchable")) {
      string tmpStr = (iter->second).screen_name; 
      s_field->addItem(QString::fromStdString(tmpStr));
      searchScoreIDs.push_back((iter->second).name);
    }
  }

  QHBoxLayout *searchLn = new QHBoxLayout;
  searchLn->addWidget(s_field);

  s_relation = new QComboBox;
  s_relation->addItem("equal");
  s_relation->addItem("include");
  s_relation->addItem("wildcard");
  s_relation->setMinimumWidth(240);
  searchLn->addWidget(s_relation);
  
  s_inputLn = new QLineEdit;
  s_inputLn->setMinimumWidth(300);
  searchLn->addWidget(s_inputLn);

  optBox1 = new QGroupBox;
  QHBoxLayout *box1_layout = new QHBoxLayout;
  searchAll = new QRadioButton("Search all Patients");
  searchPart = new QRadioButton("Search among Patients Found"); 
  searchAll->setChecked(true);
  box1_layout->addWidget(searchAll);
  box1_layout->addWidget(searchPart);
  optBox1->setLayout(box1_layout);
  optBox1->setEnabled(false);

  QGroupBox *optBox2 = new QGroupBox; 
  QHBoxLayout *box2_layout = new QHBoxLayout;
  sensButt = new QRadioButton("Case Sensitive");
  insensButt = new QRadioButton("Case Insensitive");
  sensButt->setChecked(true);
  box2_layout->addWidget(sensButt);
  box2_layout->setSpacing(10);
  box2_layout->addWidget(insensButt);
  optBox2->setLayout(box2_layout);

  QHBoxLayout *opt_layout = new QHBoxLayout;
  opt_layout->addWidget(optBox1);
  opt_layout->setSpacing(50);
  opt_layout->addWidget(optBox2);
  QVBoxLayout *s_layout = new QVBoxLayout;
  s_layout->addLayout(searchLn);
  s_layout->addLayout(opt_layout);
  spLayout->addLayout(s_layout);
}

/* Add three buttons at the bottom. */
void SearchPatient::addButtons()
{
  QPushButton *okButt = new QPushButton("&Search!");
  QPushButton *resetButt = new QPushButton("&Reset");
  QPushButton *cancelButt = new QPushButton("&Cancel");
  connect(okButt, SIGNAL(clicked()), this, SLOT(startSearch()));
  connect(resetButt, SIGNAL(clicked()), this, SLOT(clickReset()));
  connect(cancelButt, SIGNAL(clicked()), this, SLOT(clickCancel()));

  okButt->setFixedSize(80, 25);
  resetButt->setFixedSize(80, 25);
  cancelButt->setFixedSize(80, 25);

  QHBoxLayout *buttLn = new QHBoxLayout;
  buttLn->addWidget(okButt);
  buttLn->addSpacing(20);
  buttLn->addWidget(resetButt);
  buttLn->addSpacing(20);
  buttLn->addWidget(cancelButt);

  spLayout->addSpacing(20);
  spLayout->addLayout(buttLn);
}

/* Add the dock window */
void SearchPatient::setDockWindow()
{
  QDockWidget *searchDock = new QDockWidget(this);
  searchResults = new QWidget;
  searchResults->setMinimumHeight(150);
  searchDock->setWidget(searchResults);
  //searchDock->setFloating(false);
  searchDock->setFeatures(QDockWidget::NoDockWidgetFeatures);

  r_line1 = new QLabel("Search Request: NA");
  r_line2 = new QLabel("Search Results: NA");
  r_list = new QListWidget;
  connect(r_list, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(showOnePatient()));  
  QVBoxLayout *result_layout = new QVBoxLayout;
  result_layout->addWidget(r_line1);
  result_layout->addWidget(r_line2);
  result_layout->addWidget(r_list);
  //r_line1->hide();
  //r_line2->hide();
  //r_list->hide();

  searchResults->setLayout(result_layout);
  addDockWidget(Qt::BottomDockWidgetArea, searchDock);
}

/* Click "ok" will search tables in database. */
void SearchPatient::startSearch()
{
  if (s_inputLn->text().isEmpty()) {
    QMessageBox::critical(0, "Error", "Search aborted: no input string found.");
    return;
  }
  if (searchPart->isChecked() && !currentPatients.size()) {
    QMessageBox::critical(0, "Error", "Search aborted: no patients found yet.");
    return;
  }
  
  patientSearchTags ps_tags;
  setTags(ps_tags);
  vector<patientMatch> pmList;
  int search_stat = dbcp->reqSearchPatient(ps_tags, pmList);  
  if (search_stat) {
    QMessageBox::critical(0, "Error", "Patient search failed.");
    return;
  }

  currentPatients.clear();
  for (unsigned i = 0; i < pmList.size(); i++)
    currentPatients.push_back(pmList[i].patientID);

  showMatches(pmList);
}

// Set patient search tags
void SearchPatient::setTags(patientSearchTags &tags_out)
{
  int field_index = s_field->currentIndex();
  if (field_index == 0)
    tags_out.scoreName="";
  else
    tags_out.scoreName = searchScoreIDs[field_index -1];

  tags_out.relationship = s_relation->currentText().toStdString();
  tags_out.searchStr = s_inputLn->text().toStdString();

  if (insensButt->isChecked())
    tags_out.case_sensitive = false; 
  else
    tags_out.case_sensitive = true;

  if (searchPart->isChecked())
    tags_out.patientIDs = currentPatients;
}

/* This function shows search results in the dock window. */
void SearchPatient::showMatches(const vector<patientMatch>& pmList)
{
  QString reqStr = "<U>" + s_field->currentText() + " " + 
    s_relation->currentText() + " " + s_inputLn->text();
  if (insensButt->isChecked())
      reqStr.append(" (case insensitive)");
  reqStr.append("</U>"); 

  if (optBox1->isEnabled() && searchPart->isChecked())
    reqStr = r_line1->text() + " AND " + reqStr;
  else
    reqStr = "Search Request: " + reqStr;

  r_line1->setText(reqStr);
  int p_no = currentPatients.size();
  QString p_no_str;
  if (p_no < 2)
    p_no_str = QString::number(p_no) + " patient found";
  else
    p_no_str = QString::number(p_no) + " patients found";
  r_line2->setText("Search Results: " + p_no_str);

  // Remove current elements in r_list
  if (r_list->count())
    r_list->clear();

  if (p_no == 0) {
    optBox1->setEnabled(false);
    return;
  }
  // Add the current matches
  for (unsigned i = 0; i < pmList.size(); i++) {
    QString itemStr = "Patient " + QString::number(currentPatients[i]) + ": ";
    string tmpStr;
    map<string, string>::const_iterator it;
    unsigned j = 0;
    for (it = pmList[i].scoreMap.begin(); it != pmList[i].scoreMap.end(); ++it) {
      tmpStr.append(it->first);
      tmpStr.append("=");
      tmpStr.append(it->second);
      if (j < pmList[i].scoreMap.size() - 1)
        tmpStr.append(", ");
      j++;
    }
    r_list->addItem(itemStr + QString::fromStdString(tmpStr));
  }

  if (r_list->count())
    optBox1->setEnabled(true);
}

/* Click "reset" will clear up all input data on the form. */
void SearchPatient::clickReset()
{
  s_field->setCurrentIndex(0);
  s_relation->setCurrentIndex(0);
  s_inputLn->clear();
}

/* CLick "cancel" will close the curent interface. */
void SearchPatient::clickCancel()
{
  close();
}

/* This slot replies click of any patient item in list widget. */
void SearchPatient::showOnePatient()
{
  int pIndex = r_list->currentRow();
  int32 pID = currentPatients[pIndex];
  // Ask DBview to retrieve all scores of a certain patient
  //  DBpatient myPatient;
  //int stat = dbcp->reqOnePatient(pID, myPatient);
  // Make sure patient score values are retrieved successfully
  //if (stat) {
  //  QMessageBox::critical(0, "Error", "Failed to get patient score values.");
  //  return;
  //}    

  // populate DBpatient data based on lists of DBscorevalue and DBsession
  DBview *pview = new DBview(dbcp, pID);
  //pview->patient = &myPatient;
  pview->layout_view("patient");
  pview->show();
}

/* This function checks whether an input string is already an element of the input string vector.
 * If it is, return false; otherwise true. */
bool isNewElement(vector<string> inputArray, string inputStr)
{
  for (uint i = 0; i < inputArray.size(); i++) {
    if (inputArray[i] == inputStr)
      return false;
  }

  return true;
}

/* This function checks whether an input integer is already an element of the input integer vector.
 * If it is, return false; otherwise true. */
bool isNewElement(vector<int> inputArray, int inputInt)
{
  for (uint i = 0; i < inputArray.size(); i++) {
    if (inputArray[i] == inputInt)
      return false;
  }

  return true;
}

