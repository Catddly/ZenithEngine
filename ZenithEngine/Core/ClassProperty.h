#pragma once

#define ZE_NON_COPYABLE_CLASS(ClassType) ClassType(const ClassType&) = delete; \
	ClassType& operator=(const ClassType&) = delete; \
	ClassType(ClassType&&) = default; \
	ClassType& operator=(ClassType&&) = default

#define ZE_NON_COPYABLE_AND_NON_MOVABLE_CLASS(ClassType) ClassType(const ClassType&) = delete; \
	ClassType& operator=(const ClassType&) = delete; \
	ClassType(ClassType&&) = delete; \
	ClassType& operator=(ClassType&&) = delete
