/****************************************************************************
 *
 *   Copyright (c) 2015 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file data_validator.h
 *
 * A data validation class to identify anomalies in data streams
 *
 * @author Lorenz Meier <lorenz@px4.io>
 */

#pragma once

#include <cmath>
#include <stdint.h>

class __EXPORT DataValidator {
public:
	static const unsigned dimensions = 3;

	DataValidator(DataValidator *prev_sibling = nullptr);
	virtual ~DataValidator();

	/**
	 * Put an item into the validator.
	 *
	 * @param val		Item to put
	 */
	void			put(uint64_t timestamp, float val, uint64_t error_count, int priority);

	/**
	 * Put a 3D item into the validator.
	 *
	 * @param val		Item to put
	 */
	void			put(uint64_t timestamp, float val[dimensions], uint64_t error_count, int priority);

	/**
	 * Get the next sibling in the group
	 *
	 * @return		the next sibling
	 */
	DataValidator*		sibling() { return _sibling; }

	/**
	 * Get the confidence of this validator
	 * @return		the confidence between 0 and 1
	 */
	float			confidence(uint64_t timestamp);

	/**
	 * Get the error count of this validator
	 * @return		the error count
	 */
	uint64_t		error_count() { return _error_count; }

	/**
	 * Update complex error check with new values
	 * @return		True if an error is detected
	 */
	virtual void		complex_error_update() { }

	/**
	 * Check the state of the complex error checking
	 * @return		True if an error is detected
	 */
	virtual bool		complex_error_state() { return false; }

	/**
	 * Get the values of this validator
	 * @return		the stored value
	 */
	float*			value() { return _value; }

	/**
	 * Get the used status of this validator
	 * @return		true if this validator ever saw data
	 */
	bool			used() { return (_time_last > 0); }

	/**
	 * Get the priority of this validator
	 * @return		the stored priority
	 */
	int			priority() { return (_priority); }

	/**
	 * Get the error state of this validator
	 * @return		the bitmask with the error status
	 */
	uint32_t		state() { return _error_mask; }

	/**
	 * Reset the error state of this validator
	 */
	void			reset_state() { _error_mask = ERROR_FLAG_NO_ERROR; }

	/**
	 * Get the RMS values of this validator
	 * @return		the stored RMS
	 */
	float*			rms() { return _rms; }

	/**
	 * Get the vibration offset
	 * @return		the stored vibration offset
	 */
	float*			vibration_offset() { return _vibe; }

	/**
	 * Print the validator value
	 *
	 */
	virtual void		print();

	/**
	 * Set the timeout value
	 *
	 * @param timeout_interval_us The timeout interval in microseconds
	 */
	void			set_timeout(uint32_t timeout_interval_us) { _timeout_interval = timeout_interval_us; }

	/**
	 * Set the equal count threshold
	 *
	 * @param threshold The number of equal values before considering the sensor stale
	 */
	void			set_equal_value_threshold(uint32_t threshold) { _value_equal_count_threshold = threshold; }

	/**
	 * Get the timeout value
	 *
	 * @return The timeout interval in microseconds
	 */
	uint32_t			get_timeout() const { return _timeout_interval; }

	/**
	 * Data validator error states
	 */
	static constexpr uint32_t ERROR_FLAG_NO_ERROR      	= (0x00000000U);
	static constexpr uint32_t ERROR_FLAG_NO_DATA       	= (0x00000001U);
	static constexpr uint32_t ERROR_FLAG_STALE_DATA    	= (0x00000001U << 1);
	static constexpr uint32_t ERROR_FLAG_TIMEOUT 	   	= (0x00000001U << 2);
	static constexpr uint32_t ERROR_FLAG_HIGH_ERRCOUNT 	= (0x00000001U << 3);
	static constexpr uint32_t ERROR_FLAG_HIGH_ERRDENSITY 	= (0x00000001U << 4);
	static constexpr uint32_t ERROR_FLAG_COMPLEX_ERROR	= (0x00000001U << 5);

protected:
	float _value[dimensions];		/**< last value */
private:
	uint32_t _error_mask;			/**< sensor error state */
	uint32_t _timeout_interval;		/**< interval in which the datastream times out in us */
	uint64_t _time_last;			/**< last timestamp */
	uint64_t _event_count;			/**< total data counter */
	uint64_t _error_count;			/**< error count */
	int _error_density;			/**< ratio between successful reads and errors */
	int _priority;				/**< sensor nominal priority */
	float _mean[dimensions];		/**< mean of value */
	float _lp[dimensions];			/**< low pass value */
	float _M2[dimensions];			/**< RMS component value */
	float _rms[dimensions];			/**< root mean square error */
	float _vibe[dimensions];		/**< vibration level, in sensor unit */
	float _value_equal_count;		/**< equal values in a row */
	float _value_equal_count_threshold; /**< when to consider an equal count as a problem */
	DataValidator *_sibling;		/**< sibling in the group */
	static const constexpr unsigned NORETURN_ERRCOUNT = 10000;	/**< if the error count reaches this value, return sensor as invalid */
	static const constexpr float ERROR_DENSITY_WINDOW = 100.0f; 	/**< window in measurement counts for errors */
	static const constexpr unsigned VALUE_EQUAL_COUNT_DEFAULT = 100;	/**< if the sensor value is the same (accumulated also between axes) this many times, flag it */

	/* we don't want this class to be copied */
	DataValidator(const DataValidator&) = delete;
	DataValidator operator=(const DataValidator&) = delete;
};
