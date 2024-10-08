#==============================================================================
# 
#        OpenSees -- Open System For Earthquake Engineering Simulation
#                Pacific Earthquake Engineering Research Center
#
#==============================================================================

target_link_libraries(OPS_Recorder PRIVATE OPS_Actor OPS_Domain OPS_Logging)

target_sources(OPS_Recorder
    PRIVATE
      DamageRecorder.cpp
      DatastoreRecorder.cpp
      DriftRecorder.cpp
      ElementRecorder.cpp
      ElementRecorderRMS.cpp
      EnvelopeDriftRecorder.cpp
      EnvelopeElementRecorder.cpp
      EnvelopeNodeRecorder.cpp
      GSA_Recorder.cpp
      GmshRecorder.cpp
      MaxNodeDispRecorder.cpp
      NodeRecorder.cpp
      NodeRecorderRMS.cpp
      NormElementRecorder.cpp
      NormEnvelopeElementRecorder.cpp
      PatternRecorder.cpp
      Recorder.cpp
      RemoveRecorder.cpp
      VTK_Recorder.cpp
    PUBLIC
      DamageRecorder.h
      DatastoreRecorder.h
      DriftRecorder.h
      ElementRecorder.h
      ElementRecorderRMS.h
      EnvelopeDriftRecorder.h
      EnvelopeElementRecorder.h
      EnvelopeNodeRecorder.h
      GSA_Recorder.h
      GmshRecorder.h
      MaxNodeDispRecorder.h
      NodeRecorder.h
      NodeRecorderRMS.h
      NormElementRecorder.h
      NormEnvelopeElementRecorder.h
      PatternRecorder.h
      Recorder.h
      RemoveRecorder.h
      VTK_Recorder.h
)
target_sources(OPS_Paraview
    PRIVATE
      PVDRecorder.cpp
    PUBLIC
      PVDRecorder.h
)

target_sources(OPS_ASDEA
    PRIVATE
      MPCORecorder.cpp
    PUBLIC
      MPCORecorder.h
)


# target_sources(OPS_Graphics
#     PRIVATE
#       AlgorithmIncrements.cpp
#       FilePlotter.cpp
#     PUBLIC
#       AlgorithmIncrements.h
#       FilePlotter.h
# )

add_subdirectory(response)

