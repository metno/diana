GET_TARGET_PROPERTY (DIANA_BATCH_OUTPUT_NAME diana_batch OUTPUT_NAME)

SET(BDIANA_TEST_CONFIGURE_FILES
  bdiana.sh
  bdiana.setup
  field_nofail_missing.input
  field_nofail_missing.setup
  field_fail_on_missing.input
  field_fail_on_missing.setup
  obs_fail_on_error.input
  obs_fail_on_error.setup
  obs_fail_on_missing.input
  obs_fail_on_missing.setup
)

FOREACH(F ${BDIANA_TEST_CONFIGURE_FILES})
  CONFIGURE_FILE(${F}.in ${F})
ENDFOREACH()

SET(BDIANA_TEST_SCRIPTS
  field_fail_on_missing.sh
  field_nofail_missing.sh
  obs_fail_on_error.sh
  obs_fail_on_missing.sh
)
FOREACH(T ${BDIANA_TEST_SCRIPTS})
  ADD_TEST(NAME ${T} COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/${T}")
  SET_TESTS_PROPERTIES(${T}
    PROPERTIES
    ENVIRONMENT "TEST_EXTRADATA_DIR=${TEST_EXTRADATA_DIR}"
    DEPENDS diana_batch
  )
ENDFOREACH()
