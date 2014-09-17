/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

  Copyright (C) 2013 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no

  This file is part of Diana

  Diana is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Diana is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Diana; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef MODIFYLAYERSCOMMAND_H
#define MODIFYLAYERSCOMMAND_H

#include <QUndoCommand>
#include <QString>
#include <QList>
#include <QSharedPointer>

class QBitArray;

namespace EditItems {

class Layer;
class LayerManager;
class LayerGroup;

class ModifyLayersCommand : public QUndoCommand
{
public:
  ModifyLayersCommand(
      const QString &,
      LayerManager *,
      QUndoCommand * = 0);
  virtual ~ModifyLayersCommand() {}
protected:
  LayerManager *layerMgr_; // layer manager to which to apply the state change

  // layer manager state before applying the operation:
  QList<QSharedPointer<LayerGroup> > oldLayerGroups_;
  QList<QSharedPointer<Layer> > oldOrderedLayers_;

  // layer manager state after applying the operation:
  QList<QSharedPointer<LayerGroup> > newLayerGroups_;
  QList<QSharedPointer<Layer> > newOrderedLayers_;

  void removeLayers(const QList<QSharedPointer<Layer> > &);
private:
  virtual void undo();
  virtual void redo();
};

class AddEmptyLayerCommand : public ModifyLayersCommand
{
public:
  AddEmptyLayerCommand(LayerManager *, int);
};

class AddLayersCommand : public ModifyLayersCommand
{
public:
  AddLayersCommand(const QString &, LayerManager *, const QList<QSharedPointer<Layer> > &, int);
  AddLayersCommand(const QString &, LayerManager *, const QSharedPointer<Layer> &, int);
private:
  void init(const QList<QSharedPointer<Layer> > &, int);
};

class DuplicateLayersCommand : public ModifyLayersCommand
{
public:
  DuplicateLayersCommand(LayerManager *, const QBitArray &, int);
};

class RemoveLayersCommand : public ModifyLayersCommand
{
public:
  RemoveLayersCommand(LayerManager *, const QBitArray &);
};

class MergeLayersCommand : public ModifyLayersCommand
{
public:
  MergeLayersCommand(LayerManager *, const QBitArray &);
};

class ModifyLayerSelectionCommand : public ModifyLayersCommand
{
public:
  ModifyLayerSelectionCommand(LayerManager *, const QBitArray &);
};

class ModifyLayerVisibilityCommand : public ModifyLayersCommand
{
public:
  ModifyLayerVisibilityCommand(LayerManager *, const QBitArray &);
};

class MoveLayerCommand : public ModifyLayersCommand
{
public:
  MoveLayerCommand(LayerManager *, int, int);
};

} // namespace

#endif // MODIFYLAYERSCOMMAND_H
