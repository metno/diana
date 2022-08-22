/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2022 met.no

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

#ifndef DIObsReaderGRPC_H
#define DIObsReaderGRPC_H

#include "diObsReader.h"

#include "service/diObsGRPCServiceDecls.h"

class ObsReaderGRPC : public ObsReader
{
public:
  ObsReaderGRPC();
  ~ObsReaderGRPC();

  bool configure(const std::string& key, const std::string& value) override;

  bool checkForUpdates(bool useArchive) override;

  std::set<miutil::miTime> getTimes(bool useArchive, bool update) override;

  std::vector<ObsDialogInfo::Par> getParameters() override;

  PlotCommand_cpv getExtraAnnotations() override;

  void getData(ObsDataRequest_cp request, ObsDataResult_p result) override;

private:
  void updateTimes(bool useArchive);
  void updateParameters();

private:
  std::unique_ptr<diutil::grpc::obs::ObsServiceGRPCClient> client_;
  std::string name_;

  long updated_times_;
  std::set<miutil::miTime> cached_times_;

  long updated_parameters_;
  std::vector<ObsDialogInfo::Par> cached_parameters_;
};

#endif // DIObsReaderGRPC_H
