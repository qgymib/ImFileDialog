
add_executable(${PROJECT_NAME}_test
	"main.cpp"
)

target_link_libraries(${PROJECT_NAME}_test
	PRIVATE
		${PROJECT_NAME}
)

add_test(NAME ${PROJECT_NAME}_test COMMAND $<TARGET_FILE:${PROJECT_NAME}_test>)
