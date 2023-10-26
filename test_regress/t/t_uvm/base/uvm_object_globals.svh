// SPDX-License-Identifier: Apache-2.0
//
//------------------------------------------------------------------------------
// Copyright 2007-2011 Mentor Graphics Corporation
// Copyright 2017 Intel Corporation
// Copyright 2010 Synopsys, Inc.
// Copyright 2007-2018 Cadence Design Systems, Inc.
// Copyright 2010 AMD
// Copyright 2015-2018 NVIDIA Corporation
//   All Rights Reserved Worldwide
//
//   Licensed under the Apache License, Version 2.0 (the
//   "License"); you may not use this file except in
//   compliance with the License.  You may obtain a copy of
//   the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in
//   writing, software distributed under the License is
//   distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
//   CONDITIONS OF ANY KIND, either express or implied.  See
//   the License for the specific language governing
//   permissions and limitations under the License.
//------------------------------------------------------------------------------

typedef enum { UVM_PHASE_UNINITIALIZED = 0,
               UVM_PHASE_DORMANT       = 1,
               UVM_PHASE_SCHEDULED     = 2,
               UVM_PHASE_SYNCING       = 4,
               UVM_PHASE_STARTED       = 8,
               UVM_PHASE_EXECUTING     = 16,
               UVM_PHASE_READY_TO_END  = 32,
               UVM_PHASE_ENDED         = 64,
               UVM_PHASE_CLEANUP       = 128,
               UVM_PHASE_DONE          = 256,
               UVM_PHASE_JUMPING       = 512
               } uvm_phase_state;

typedef enum { UVM_PHASE_IMP,
               UVM_PHASE_NODE,
               UVM_PHASE_TERMINAL,
               UVM_PHASE_SCHEDULE,
               UVM_PHASE_DOMAIN,
               UVM_PHASE_GLOBAL
} uvm_phase_type;
