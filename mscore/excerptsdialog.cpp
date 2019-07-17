//=============================================================================
//  MuseScore
//  Linux Music Score Editor
//
//  Copyright (C) 2008 Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "excerptsdialog.h"
#include "musescore.h"
#include "libmscore/score.h"
#include "libmscore/part.h"
#include "libmscore/undo.h"
#include "icons.h"

namespace Ms {

//MARK:- ExcerptItem class (subclass of QListWidgetItem)
//---------------------------------------------------------
//   ExcerptItem
//---------------------------------------------------------

ExcerptItem::ExcerptItem(Excerpt* e, QListWidget* parent)
   : QListWidgetItem(parent)
      {
      _excerpt = e;
      setText(e->title());
      setFlags(flags() | Qt::ItemIsEditable);

      QIcon icon;

      if (isEditable())
            icon.addFile(QString::fromUtf8(":/data/icons/edit.svg"), QSize(12,12), QIcon::Normal, QIcon::Off);
      else
            icon.addFile(QString::fromUtf8(":/data/icons/document.svg"), QSize(12,12), QIcon::Normal, QIcon::Off);

      setIcon(icon);
      }

bool ExcerptItem::isEditable()
      {
      return _excerpt->partScore() == 0;
      }

//MARK:- PartItem class (subclass of QTreeWidgetItem)

//---------------------------------------------------------
//   PartItem
//---------------------------------------------------------

PartItem::PartItem(Part* p, QTreeWidget* parent)
   : QTreeWidgetItem(parent)
      {
      setEditable(true);
      _part   = p;
      setText(0, p->partName().replace("/", "_"));
      }

void PartItem::setEditable(bool editable)
      {
      if (!editable) {
            setFlags(Qt::ItemFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable));
            return;
            }

      setFlags(Qt::ItemFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable));
      }

//MARK:- InstrumentItem class (subclass of QListWidgetItem)

//---------------------------------------------------------
//   InstrumentItem
//---------------------------------------------------------

InstrumentItem::InstrumentItem(PartItem* p, QListWidget* parent)
   : QListWidgetItem(parent)
      {
      setFlags(Qt::ItemFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable));
      _partItem = p;
      setText(p->part()->partName().replace("/", "_"));
      }


//MARK:- StaffItem class (subclass of QTreeWidgetItem)

//---------------------------------------------------------
//   StaffItem
//---------------------------------------------------------

StaffItem::StaffItem(PartItem* li)
   : QTreeWidgetItem(li)
      {
      setFlags(Qt::ItemFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable));
      for (int i = 1; i <= VOICES; i++) {
            setCheckState(i, Qt::Checked);
            }
      }

void StaffItem::setData(int column, int role, const QVariant& value)
      {
      const bool isCheckChange = column > 0
         && role == Qt::CheckStateRole
         && data(column, role).isValid() // Don't "change" during initialization
         && checkState(column) != value;
      QTreeWidgetItem::setData(column, role, value);
      if (isCheckChange) {
            int unchecked = 0;
            for (int i = 1; i <= VOICES; i++) {
                  if (checkState(i) == Qt::Unchecked)
                        unchecked += 1;
                  }
            if (unchecked == VOICES)
                  setCheckState(column, Qt::Checked);
            }
      }

//MARK:- ExcerptsDialog constructor and setup

//---------------------------------------------------------
//   ExcerptsDialog
//---------------------------------------------------------

ExcerptsDialog::ExcerptsDialog(MasterScore* s, QWidget* parent)
   : QDialog(parent)
      {
      setObjectName("PartEditor");
      setupUi(this);
      setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
      setModal(true);

      score = s->masterScore();

      for (Excerpt* e : score->excerpts()) {
            ExcerptItem* ei = new ExcerptItem(e);
            excerptList->addItem(ei);
            }
      QMultiMap<int, int> t;
      for (Part* p : score->parts()) {
            PartItem* pI = new PartItem(p);
            InstrumentItem* item = new InstrumentItem(pI);
            instrumentList->addItem(item);
            }

      instrumentList->setSelectionMode(QAbstractItemView::ExtendedSelection);

      excerptList->setIconSize(QSize(12, 12));

      partList->header()->setSectionResizeMode(0, QHeaderView::Stretch);
      partList->header()->setSectionResizeMode(1, QHeaderView::Fixed);
      partList->header()->setSectionResizeMode(2, QHeaderView::Fixed);
      partList->header()->setSectionResizeMode(3, QHeaderView::Fixed);
      partList->header()->setSectionResizeMode(4, QHeaderView::Fixed);

      connect(newEmptyPartButton, SIGNAL(clicked()), SLOT(newClicked()));
      connect(newPartForInstrumentButton, SIGNAL(clicked()), SLOT(newForInstrumentClicked()));

      connect(newAllButton, SIGNAL(clicked()), SLOT(newAllClicked()));
      connect(deleteButton, SIGNAL(clicked()), SLOT(deleteClicked()));
      connect(moveUpButton, SIGNAL(clicked()), SLOT(moveUpClicked()));
      connect(moveDownButton, SIGNAL(clicked()), SLOT(moveDownClicked()));
      connect(addButton, SIGNAL(clicked()), SLOT(addButtonClicked()));
      connect(removeButton, SIGNAL(clicked()), SLOT(removeButtonClicked()));

      connect(excerptList, SIGNAL(itemChanged(QListWidgetItem*)), SLOT(titleChanged(QListWidgetItem*)));

      connect(excerptList, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
         SLOT(excerptChanged(QListWidgetItem*, QListWidgetItem*)));
      connect(partList, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
         SLOT(partDoubleClicked(QTreeWidgetItem*, int)));
      connect(partList, SIGNAL(itemClicked(QTreeWidgetItem*,int)), SLOT(partClicked(QTreeWidgetItem*,int)));
      connect(instrumentList, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
              SLOT(doubleClickedInstrument(QListWidgetItem*)));
      connect(instrumentList, SIGNAL(itemSelectionChanged()), SLOT(instrumentListSelectionChanged()));

      moveUpButton->setIcon(*icons[int(Icons::arrowUp_ICON)]);
      moveDownButton->setIcon(*icons[int(Icons::arrowDown_ICON)]);
      
      for (int i = 1; i <= VOICES; i++) {
            //partList->model()->setHeaderData(i, Qt::Horizontal, MScore::selectColor[i-1], Qt::BackgroundRole);
            partList->header()->resizeSection(i, 30);
            }

      int n = score->excerpts().size();
      if (n > 0)
            excerptList->setCurrentRow(0);
      moveDownButton->setEnabled(n > 1);
      moveUpButton->setEnabled(false);
      bool flag = excerptList->currentItem() != 0;
      deleteButton->setEnabled(flag);
      }

//---------------------------------------------------------
//   startExcerptsDialog
//---------------------------------------------------------

void MuseScore::startExcerptsDialog()
      {
      if (cs == 0)
            return;
      ExcerptsDialog ed(cs->masterScore(), 0);
      MuseScore::restoreGeometry(&ed);
      ed.exec();
      MuseScore::saveGeometry(&ed);
      cs->setLayoutAll();
      cs->update();
      }

//MARK:- Processing other misc signals / enabling & disabling


ExcerptItem* ExcerptsDialog::selectedExcerpt()
      {
      
      if (excerptList->selectedItems().count() == 0) {
            qDebug()<<"returning nullptr from selectedExcerpt";
            return nullptr;
       }

      return static_cast<ExcerptItem*>(excerptList->selectedItems().first());
      }

void ExcerptsDialog::instrumentListSelectionChanged()
      {
      // selection has just changed - disable buttons if nothing selected
      qDebug()<<"instrumentListSelectionChanged()";
      bool someSelection = instrumentList->selectedItems().count();

      addButton->setEnabled(selectedExcerpt() ? selectedExcerpt()->isEditable() : false);
      newPartForInstrumentButton->setEnabled(someSelection);
      }

//MARK:- Creating new excepts
//---------------------------------------------------------
//   newClicked
//---------------------------------------------------------

void ExcerptsDialog::newClicked()
      {
      makeNewExcerpt();
      }


void ExcerptsDialog::makeNewExcerpt()
      {
      QString name = createName("Part");
            Excerpt* e   = new Excerpt(score);
            e->setTitle(name);
            ExcerptItem* ei = new ExcerptItem(e);
            excerptList->addItem(ei);
            excerptList->selectionModel()->clearSelection();
            excerptList->setCurrentItem(ei, QItemSelectionModel::SelectCurrent);
            for (int i = 0; i < excerptList->count(); ++i) {
                  ExcerptItem* eii = (ExcerptItem*)excerptList->item(i);
                  if (eii->excerpt()->title() != eii->text()) {
                        // if except score not created yet, change the UI title
                        // if already created, change back(see createName) the excerpt title
                        if (!eii->excerpt()->partScore())
                              eii->setText(eii->excerpt()->title());
                        else
                              eii->excerpt()->setTitle(eii->text());
                        }
                  }
      }

void ExcerptsDialog::newForInstrumentClicked()
      {
      qDebug()<<"newForInstrumentClicked";

      makeNewExcerpt(); // will create a new excerpt / part

      // find selected instrument
      QString nameSuggestion = addSelectedInstrumentToCurrentExcerpt();

      instrumentList->selectionModel()->clearSelection();
      Excerpt* cur = ((ExcerptItem*)(excerptList->currentItem()))->excerpt();
      cur->setTitle(nameSuggestion);
      excerptList->currentItem()->setText(nameSuggestion);
      }
//---------------------------------------------------------
//   newAllClicked
//---------------------------------------------------------

void ExcerptsDialog::newAllClicked()
      {
      QList<Excerpt*> excerpts = Excerpt::createAllExcerpt(score);
      ExcerptItem* ei = 0;
      for (Excerpt* e : excerpts) {
            ei = new ExcerptItem(e);
            excerptList->addItem(ei);
            }
      if (ei) {
            excerptList->selectionModel()->clearSelection();
            excerptList->setCurrentItem(ei, QItemSelectionModel::SelectCurrent);
            }
      }


//MARK:- Moving and deleting excerpts

//---------------------------------------------------------
//   deleteClicked
//---------------------------------------------------------

void ExcerptsDialog::deleteClicked()
      {
      QListWidgetItem* cur = excerptList->currentItem();
      if (cur == 0)
            return;

      delete cur;
      }

//---------------------------------------------------------
//   moveUpClicked
//---------------------------------------------------------

void ExcerptsDialog::moveUpClicked()
      {
      QListWidgetItem* cur = excerptList->currentItem();
      if (cur == 0)
            return;
      int currentRow = excerptList->currentRow();
      if (currentRow <= 0)
            return;
      QListWidgetItem* currentItem = excerptList->takeItem(currentRow);
      excerptList->insertItem(currentRow - 1, currentItem);
      excerptList->setCurrentRow(currentRow - 1);
      }

//---------------------------------------------------------
//   moveDownClicked
//---------------------------------------------------------

void ExcerptsDialog::moveDownClicked()
      {
      QListWidgetItem* cur = excerptList->currentItem();
      if (cur == 0)
            return;
      int currentRow = excerptList->currentRow();
      int nbRows = excerptList->count();
      if (currentRow >= nbRows - 1)
            return;
      QListWidgetItem* currentItem = excerptList->takeItem(currentRow);
      excerptList->insertItem(currentRow + 1, currentItem);
      excerptList->setCurrentRow(currentRow + 1);
      }

//MARK:- adding instruments (?)

//---------------------------------------------------------
//  addButtonClicked
//    add instrument to excerpt
//---------------------------------------------------------

void ExcerptsDialog::addButtonClicked()
      {
      addSelectedInstrumentToCurrentExcerpt();
      instrumentList->selectionModel()->clearSelection();
      
      }

QString ExcerptsDialog::addSelectedInstrumentToCurrentExcerpt()
      {
      if (!excerptList->currentItem() || !partList->isEnabled())
            return QString("");

      Excerpt* cur = ((ExcerptItem*)(excerptList->currentItem()))->excerpt();

      QList<QString> instrumentNames;

      for (QListWidgetItem* i : instrumentList->selectedItems()) {
            InstrumentItem* item = static_cast<InstrumentItem*>(i);
            const PartItem* it   = item->partItem();
            if (it == 0)
                  continue;
            PartItem* pi = new PartItem(it->part(), 0);
            pi->setText(0, pi->part()->name());
            cur->parts().append(pi->part());
            partList->addTopLevelItem(pi);
            for (Staff* s : *pi->part()->staves()) {
                  StaffItem* sli = new StaffItem(pi);
                  sli->setStaff(s);
                  for (int j = 0; j < VOICES; j++)
                        sli->setCheckState(j + 1, Qt::Checked);
                  }
            QString instrumentName = pi->part()->partName();
            pi->setText(0, instrumentName);
            instrumentNames.append(instrumentName);

            }

      cur->setTracks(mapTracks());
      partList->resizeColumnToContents(0);

      return excerptNameFromListOfNames(instrumentNames);
      }

QString ExcerptsDialog::excerptNameFromListOfNames(QList<QString> names)
      {
      //TODO: check whether name is already used and add a number to differentiate?
      // can MuseScore cope with two names being the same? Even if it can, it's
      // classier to add some differentiator to let the user know. In existing code
      // there's no check for changing the title to something already used, i.e. user
      // can force two names the same if they want in old code.

      QString candidateName = "Part";

      if (names.count() == 0)
            qDebug()<<"ExcerptsDialog::excerptNameFromListOfNames() - empty list of names. Using \"Part\"";
      else if (names.count() == 1)
            candidateName = names.first();
      else if (names.count() == 2)
            candidateName = QString(tr("%1 & %2")).arg(names.first()).arg(names.last());
      else
            candidateName = QString(tr("%1, %2 etc.")).arg(names[0]).arg(names[1]);

//      int nameCount = 1;
//      for (int listIndex; listIndex < excerptList->count(); listIndex++) {
//            // need to check if there's a suffix already and if there is
//            // ensure nameCount is at least is at least one higher than that
//            // suffix and then set aside the suffix before doing the comparison
//            // smart thing to do would be to use a regular expression

              // or take a different approach and manage in the item itself
//            }

      return candidateName;
      }


//---------------------------------------------------------
//   removeButtonClicked
//    remove instrument from score
//---------------------------------------------------------

void ExcerptsDialog::removeButtonClicked()
      {
      QList<QTreeWidgetItem*> wi = partList->selectedItems();
      if (wi.isEmpty())
            return;

      Excerpt* cur = ((ExcerptItem*)(excerptList->currentItem()))->excerpt();
      QTreeWidgetItem* item = wi.first();

      cur->parts().removeAt(partList->indexOfTopLevelItem(item));
      delete item;

      cur->setTracks(mapTracks());
      partList->resizeColumnToContents(0);
      }

//---------------------------------------------------------
//   excerptChanged
//---------------------------------------------------------

void ExcerptsDialog::excerptChanged(QListWidgetItem* currentItem, QListWidgetItem*)
      {
      bool currentItemIsEditable = true;

      if (currentItem) {
            ExcerptItem* curItem = static_cast<ExcerptItem*>(currentItem);
            Excerpt* excerpt = curItem->excerpt();

            // the & character has a special meaning in QWidget titles
            QString adjustedTitle = curItem->text().replace("&", "&&");
            QString groupBoxTitle = QString(tr("Instruments in: %1")).arg(adjustedTitle);
            detailsGroupBox->setTitle(groupBoxTitle);

            currentItemIsEditable = curItem->isEditable();

            // set selection:
            QList<Part*>& partListForExcerpt = excerpt->parts();
            QMultiMap<int, int> tracks = excerpt->tracks();

            // display the details (instruments added, staves selected) for the part
            // and enable or disable according to whether the part is editable
            partList->clear();
            for (Part* partWithinExcerpt: partListForExcerpt) {
                  PartItem* partItem = new PartItem(partWithinExcerpt, partList);
                  partItem->setEditable(currentItemIsEditable);

                  partList->addTopLevelItem(partItem);

                  int staffIndex = 1;
                  for (Staff* staff : *partWithinExcerpt->staves()) {
                        StaffItem* staffItem = new StaffItem(partItem);
                        staffItem->setStaff(staff);
                        staffItem->setDisabled(!currentItemIsEditable);
                        staffItem->setText(0, tr("Staff %1").arg(staffIndex));
                        staffIndex++;
                        }
                  partItem->setText(0, partWithinExcerpt->partName());
                  partList->setItemExpanded(partItem, false);
                  }
            assignTracks(tracks);
            }
      else {
            detailsGroupBox->setTitle(tr("Instruments in:"));
            partList->clear();
            currentItemIsEditable = false;
            }
      addButton->setEnabled(currentItemIsEditable);
      removeButton->setEnabled(currentItemIsEditable);

      bool flag = excerptList->currentItem() != 0;
      int n = excerptList->count();
      int idx = excerptList->currentIndex().row();
      moveUpButton->setEnabled(idx > 0);
      moveDownButton->setEnabled(idx < (n-1));
      deleteButton->setEnabled(flag);
      }

//---------------------------------------------------------
//   partDoubleClicked
//---------------------------------------------------------

void ExcerptsDialog::partDoubleClicked(QTreeWidgetItem* item, int)
      {
      if (!item->parent()) { // top level items are PartItem
            PartItem* pi = (PartItem*)item;
            QString instrumentName = pi->part()->partName();
            ExcerptItem* selectedPart = static_cast<ExcerptItem*>(excerptList->currentItem());
            selectedPart->setText(instrumentName);
            selectedPart->excerpt()->setTitle(instrumentName);
            qDebug()<<"Double click on the instrument name in part details sets the title to the name of that instrument. proposed new title is:"<<instrumentName;
            }
      }

//---------------------------------------------------------
//   partClicked
//---------------------------------------------------------

void ExcerptsDialog::partClicked(QTreeWidgetItem*, int)
      {
      QListWidgetItem* cur = excerptList->currentItem();
      if (cur == 0)
            return;

      Excerpt* excerpt = static_cast<ExcerptItem*>(cur)->excerpt();
      excerpt->setTracks(mapTracks());
      }

//---------------------------------------------------------
//   doubleClickedInstrument
//---------------------------------------------------------

void ExcerptsDialog::doubleClickedInstrument(QListWidgetItem*)
      {
      if (selectedExcerpt() ? selectedExcerpt()->isEditable() : false)
            addButtonClicked();
      }

//---------------------------------------------------------
//   titleChanged
//---------------------------------------------------------

void ExcerptsDialog::titleChanged(QListWidgetItem* item)
      {
      ExcerptItem* cur = static_cast<ExcerptItem*>(item);
      if (cur == 0)
            return;
      cur->excerpt()->setTitle(item->text());
      excerptChanged(cur, nullptr);
      }

//---------------------------------------------------------
//   createName
//---------------------------------------------------------

QString ExcerptsDialog::createName(const QString& partName)
      {
      int count = excerptList->count();
      QList<Excerpt*> excerpts;
      for (int i = 0; i < count; ++i) {
            Excerpt* ee = static_cast<ExcerptItem*>(excerptList->item(i))->excerpt();
            excerpts.append(ee);
            }
      return Excerpt::createName(partName, excerpts);
      }

//---------------------------------------------------------
//   mapTracks
//---------------------------------------------------------

QMultiMap<int, int> ExcerptsDialog::mapTracks()
      {
      QMultiMap<int, int> tracks;
      int track = 0;

      for (QTreeWidgetItem* pwi = partList->itemAt(0,0); pwi; pwi = partList->itemBelow(pwi)) {
            PartItem* pi = (PartItem*)pwi;
            Part* p = pi->part();
            for (int j = 0; j < pwi->childCount(); j++) {
                  for (int k = 0; k < VOICES; k++) {
                        if (pwi->child(j)->checkState(k+1) == Qt::Checked) {
                              int voiceOff = 0;
                              int srcTrack = p->startTrack() + j * VOICES + k;
                              for (int i = srcTrack & ~3; i < srcTrack; i++) {
                                    QList<int> t = tracks.values(i);
                                    for (int ti : t) {
                                          if (ti >= (track & ~3))
                                                voiceOff++;
                                          }
                                    }
                              tracks.insert(srcTrack, (track & ~3) + voiceOff);
                              }
                        track++;
                        }
                  }
            }
      return tracks;
      }

//---------------------------------------------------------
//   assignTracks
//---------------------------------------------------------

void ExcerptsDialog::assignTracks(QMultiMap<int, int> tracks)
      {
      int track = 0;
      for (QTreeWidgetItem* pwi = partList->itemAt(0,0); pwi; pwi = partList->itemBelow(pwi)) {
            for (int j = 0; j < pwi->childCount(); j++) {
                  for (int k = 0; k < VOICES; k++) {
                        int checkTrack = tracks.key(track, -1);
                        if (checkTrack != -1)
                              pwi->child(j)->setCheckState((checkTrack % VOICES) + 1, Qt::Checked);
                        else
                              pwi->child(j)->setCheckState((track % VOICES) + 1, Qt::Unchecked);
                        track++;
                        }
                  }
            }
      }

//---------------------------------------------------------
//   isInPartList
//---------------------------------------------------------

ExcerptItem* ExcerptsDialog::isInPartsList(Excerpt* e)
      {
      int n = excerptList->count();
      for (int i = 0; i < n; ++i) {
            excerptList->setCurrentRow(i);
            // ExcerptItem* cur = static_cast<ExcerptItem*>(excerptList->currentItem());
            ExcerptItem* cur = static_cast<ExcerptItem*>(excerptList->item(i));
            if (cur == 0)
                  continue;
//            if (((ExcerptItem*)cur)->excerpt() == ExcerptItem(e).excerpt())
            if (cur->excerpt() == e)
                  return cur;
            }
      return 0;
      }

//---------------------------------------------------------
//   createExcerpt
//---------------------------------------------------------

void ExcerptsDialog::createExcerptClicked(QListWidgetItem* cur)
      {
      Excerpt* e = static_cast<ExcerptItem*>(cur)->excerpt();
      if (e->partScore())
            return;
      if (e->parts().isEmpty()) {
            qDebug("no parts");
            return;
            }

      Score* nscore = new Score(e->oscore());
      e->setPartScore(nscore);

      qDebug() << " + Add part : " << e->title();
      score->undo(new AddExcerpt(e));
      Excerpt::createExcerpt(e);

      // a new excerpt is created in AddExcerpt, make sure the parts are filed
      for (Excerpt* ee : e->oscore()->excerpts()) {
            if (ee->partScore() == nscore && ee != e) {
                  ee->parts().clear();
                  ee->parts().append(e->parts());
                  }
            }

      partList->setEnabled(false);
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void ExcerptsDialog::accept()
      {
      score->startCmd();

      // first pass : see if actual parts needs to be deleted
      foreach (Excerpt* e, score->excerpts()) {
            Score* partScore  = e->partScore();
            ExcerptItem* item = isInPartsList(e);
            if (!isInPartsList(e) && partScore)      // Delete it because not in the list anymore
                  score->deleteExcerpt(e);
            else {
                  if (item->text() != e->title())
                        score->undo(new ChangeExcerptTitle(e, item->text()));
                  }
            }
      // Second pass : Create new parts
      int n = excerptList->count();
      for (int i = 0; i < n; ++i) {
            excerptList->setCurrentRow(i);
            QListWidgetItem* cur = excerptList->currentItem();
            if (cur == 0)
                  continue;
            createExcerptClicked(cur);
            }

      // Third pass : Remove empty parts.
      int i = 0;
      while (i < excerptList->count()) {
            // This new part is empty, so we don't create an excerpt but remove it from the list.
            // Necessary to order the parts later on.
            excerptList->setCurrentRow(i);
            QListWidgetItem* cur = excerptList->currentItem();
            Excerpt* e = static_cast<ExcerptItem*>(cur)->excerpt();
            if (e->parts().isEmpty() && !e->partScore()) {
                  qDebug() << " - Deleting empty parts : " << cur->text();
                  delete cur;
                  }
            else
                  i++;
            }

      // Update the score parts order following excerpList widget
      // The reference is the excerpt list. So we iterate following it and swap parts in the score accordingly

      for (int j = 0; j < excerptList->count(); ++j) {
            excerptList->setCurrentRow(j);
            QListWidgetItem* cur = excerptList->currentItem();
            if (cur == 0)
                  continue;

            int position = 0;  // Actual order position in score
            bool found = false;
            Excerpt* foundExcerpt;
            // Looks for the excerpt and its position.
            foreach(Excerpt* e, score->excerpts()) {
                  if (((ExcerptItem*)cur)->excerpt() == ExcerptItem(e).excerpt()) {
                        found = true;
                        foundExcerpt = ExcerptItem(e).excerpt();
                        break;
                        }
                  position++;
                  }
            if ((found) && (position != j))
                  score->undo(new SwapExcerpt(score, j, position));
            else if (found && cur->text() != foundExcerpt->title()) {
                  score->undo(new ChangeExcerptTitle(foundExcerpt, cur->text()));
                  qDebug()<<"change of name, but not order - new code attemptng to fix";
                  score->setExcerptsChanged(true);
                  }
            }
      score->endCmd();
      QDialog::accept();
      }
}
