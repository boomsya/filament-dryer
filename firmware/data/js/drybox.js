var chart, chart11, chart12;
var status_reading_now = false;

$(document).ready(function () {
	//Get checkbox statuses from localStorage if available (IE)
	if (localStorage) {
		//Menu minifier status (Contract/expand side menu on large screens)
		if (localStorage.getItem('minifier') === 'true') {
			$('#sidebar,#menu-minifier').addClass('mini');
			$('#minifier').prop('checked', true);
		} else {
			if ($('#minifier').is(':checked')) {
				$('#sidebar,#menu-minifier').addClass('mini');
				$('#minifier').prop('checked', true);
			} else {
				$('#sidebar,#menu-minifier').removeClass('mini');
				$('#minifier').prop('checked', false);
			}
		}
	}

	//Contract/expand side menu on click. (only large screens)
	$('#minifier').click(function () {
		$('#sidebar,#menu-minifier').toggleClass('mini');
		// Save side menu status to localStorage if available (IE)
		if (localStorage) {
			localStorage.setItem('minifier', this.checked);
		}
	});

	// Side menu toogler for medium and small screens
	$('[data-toggle="offcanvas"]').click(function () {
		$('.row-offcanvas').toggleClass('active');
	});

	//Hide menu after item click
	$('a.nav-link').click(function () {
		$('.row-offcanvas').removeClass('active');
	});

	// Switch (checkbox element) toogler
	$('.switch input[type="checkbox"]').on("change", function (t) {
		// Check the time between changes to prevert Android native browser execute twice
		// If you don`t need support for Android native browser - just call "switchSingle" function
		if (this.last) {
			this.diff = t.timeStamp - this.last;
			// Don't execute if the time between changes is too short (less than 250ms) - Android native browser "twice bug"
			// The real time between two human taps/clicks is usually much more than 250ms"
			if (this.diff > 250) {
				this.last = t.timeStamp;
				iot.switchSingle(this.id, this.checked);
				setparams();
			} else {
				return false;
			}
		} else {
			// First attempt on this switch element
			this.last = t.timeStamp;
			iot.switchSingle(this.id, this.checked);
			setparams();
		}
	});

	// Show/hide tips (popovers) - FAB button (right bottom on large screens)
	$('#info-toggler').click(function () {
		if ($('body').hasClass('info-active')) {
			$('[data-toggle="popover-all"]').popover('hide');
			$('body').removeClass('info-active');
		} else {
			$('[data-toggle="popover-all"]').popover('show');
			$('body').addClass('info-active');
		}
	});

	// Hide tips (popovers) by clicking outside
	$('body').on('click', function (pop) {
		if (pop.target.id !== 'info-toggler' && $('body').hasClass('info-active')) {
			$('[data-toggle="popover-all"]').popover('hide');
			$('body').removeClass('info-active');
		}
	});

	// Data binding for numeric representation of TV Volume range slider
	$(document).on('input', 'input[type="range"].volume', function () {
		$('[data-rangeslider="' + this.id + '"] .range-output').html(this.value);
	});
});

// Apply necessary changes, functionality when content is loaded
$(window).on('load', function () {
	// This script is necessary for cross browsers icon sprite support (IE9+, ...)
	svg4everybody();

	//apexcharts
	let options= {
		series: [
			{
				name: 'Heater temperature',
				type: 'line',
				data: [null, null, null, null, null, null, null, null, null, null, null, null, null, null, null]
			},
			{
				name: 'Temperature inside',
				type: 'line',
				data: [null, null, null, null, null, null, null, null, null, null, null, null, null, null, null]
			}, {
				name: 'Temperature outside',
				type: 'line',
				data: [null, null, null, null, null, null, null, null, null, null, null, null, null, null, null]
			},
			{
				name: 'Humidity inside',
				type: 'line',
				data: [null, null, null, null, null, null, null, null, null, null, null, null, null, null, null]
			},
			{
				name: 'Humidity outside',
				type: 'line',
				data: [null, null, null, null, null, null, null, null, null, null, null, null, null, null, null]
			}
		],
		chart: {
			animations: {
				enabled: false
			},
			selection: {
				enabled: false
			},
			foreColor: 'rgba(15,156,230,.6)',
			height: 350,
			type: 'line',
			stacked: false,
			zoom: {
				enabled: false
			},
			toolbar: {
				show: false
			}
		},
		grid: {
			strokeDashArray: 2,
			borderColor: 'rgba(15,156,230,.6)',
			xaxis: {
				lines: {
					show: true
				}
			},
		},
		colors: ['#ac0c46', '#b7713f', '#2575c4', '#319600', '#ff66ff'],
		dataLabels: {
			enabled: false
		},
		stroke: {
			curve: 'smooth',
			width: [6, 6, 3, 3, 3]
		},
		markers: {
			strokeColors: '#1c2e3b',
		},
		xaxis: {
			axisTicks: {
				show: false,
			},
			axisBorder: {
				show: false
			},
			tooltip: {
				enabled: false
			},
			categories: ['-3:30h', '-3:15h', '-3:00h', '-2:45h', '-2:30h', '-2:15h', '-2:00h', '-1:45h', '-1:30h', '-1:15h', '-1:00h', '-0:45h', '-0:30h', '-0:15h', 'Now'],
		},
		yaxis: [
			{
				seriesName: 'Heater temperature',
				axisTicks: {
					show: false,
				},
				axisBorder: {
					show: false,
				},
				labels: {
					style: {
						fontSize: "12px",
						colors: '#dd004a',
					},
					formatter: function(val, index) {
						return (val==null) ? '' : val.toFixed(0)+'°C';
					}
				},
				title: {
					text: "Temperature",
					style: {
						fontWeight: 500,
						fontSize: "18px",
						color: '#dd004a',
					}
				},
				tooltip: {
					enabled: false
				},
			},
			{
				seriesName: 'Heater temperature',
				show: false
			},
			{
				seriesName: 'Heater temperature',
				show: false
			},
			{
				seriesName: 'Humidity inside',
				opposite: true,
				axisTicks: {
					show: false,
				},
				axisBorder: {
					show: false,
				},
				labels: {
					style: {
						fontSize: "12px",
						colors: 'rgba(15,156,230,.75)',
					},
					formatter: function(val, index) {
						return (val==null) ? '' : val.toFixed(0)+'%';
					}
				},
				title: {
					text: "Humidity",
					style: {
						fontSize: "18px",
						fontWeight: 500,
						color: 'rgba(15,156,230,.75)',
					}
				}
			},
			{
				seriesName: 'Humidity inside',
				show: false
			}
		],
		annotations: {
			yaxis: [
				{
					y: 45,
					borderColor: '#e38800',
					label: {
						borderColor: '#e38800',
						offsetY: 7,
						offsetX: -8,
						style: {
							color: '#fff',
							fontSize: "12px",
							background: '#e38800',
						},
						text: 'Target temperature',
					}
				},
				{
					y: 55,
					borderColor: '#f00',
					label: {
						borderColor: '#f00',
						offsetY: 7,
						offsetX: -8,
						style: {
							color: '#fff',
							fontSize: "12px",
							background: '#f00',
						},
						text: 'Max Heater Temperature',
					}
				}
			]
		},
		tooltip: {
			theme: 'dark',
			shared: true,
			fixed: {
				enabled: false,
				position: 'topLeft', // topRight, topLeft, bottomRight, bottomLeft
				offsetY: 30,
				offsetX: 60
			},
		},
		legend: {
			fontSize: '16px',
			horizontalAlign: 'left',
			offsetX: 55,
			markers: {
				radius: 2,
			},
			itemMargin: {
				horizontal: 10,
			},
		}
	};

	chart = new ApexCharts(document.querySelector("#chart02"), options);
	chart.render();

	//radial chart "Temperature in"
	let options11 = {
		series: [0],
		chart: {
			type: 'radialBar',
			offsetY: -10,
			sparkline: {
				enabled: true
			}
		},
		plotOptions: {
			radialBar: {
				startAngle: -90,
				endAngle: 90,
				track: {
					background: "#493b33",
					strokeWidth: '97%',
					margin: 5, //is in pixels
					dropShadow: {
						enabled: false,
					}
				},
				dataLabels: {
					name: {
						show: true,
						offsetY: 25,
						color: '#0f9ce6',
						fontSize: '20px',
						fontWeight: 500
					},
					value: {
						offsetY: -18,
						formatter: function(val) {
							if (val == 0) {
								return '-- °C';
							}else {
								return val + ' °C';
							}
						},
						color: '#aee0fa',
						fontSize: '22px',
					}
				}
			}
		},
		grid: {
			padding: {
				top: -10
			}
		},
		fill: {
			type: 'solid',
			colors: ['#f00']
		},
		labels: ['Temperature in'],
	};

	chart11 = new ApexCharts(document.querySelector("#chart11"), options11);
	chart11.render();

	//radial chart "Humidity in"
	let options12 = {
		series: [0],
		chart: {
			type: 'radialBar',
			offsetY: -10,
			sparkline: {
				enabled: true
			}
		},
		plotOptions: {
			radialBar: {
				startAngle: -90,
				endAngle: 90,
				track: {
					background: "#10425e",
					strokeWidth: '97%',
					margin: 5, //is in pixels
					dropShadow: {
						enabled: false,
					}
				},
				dataLabels: {
					name: {
						show: true,
						offsetY: 25,
						color: '#0f9ce6',
						fontSize: '20px',
						fontWeight: 500
					},
					value: {
						offsetY: -18,
						formatter: function(val) {
							if (val == 0) {
								return '-- %';
							}else {
								return val + ' %';
							}
						},
						color: '#aee0fa',
						fontSize: '22px',
					}
				}
			}
		},
		grid: {
			padding: {
				top: -10
			}
		},
		fill: {
			type: 'solid',
			colors: ['#0f9ce6']
		},
		labels: ['Humidity in'],
	};

	chart12 = new ApexCharts(document.querySelector("#chart12"), options12);
	chart12.render();

	// "Timeout" function is not necessary - important is to hide the preloader overlay
	setTimeout(function () {
		// Initialize range sliders
		$('input[type="range"]').rangeslider({
			polyfill: false,
			onSlideEnd: function (position, value) {
				setparams();
			}
		}).change();

		//Control STOP clicked
		$('#stop').on('click', function () {
			$.ajax({
				url: "/off",
				type: "get",
				success: function (response) {
					update_status(false);
				},
				error: function (xhr) {
					lock_ui(true);
				}
			});
		});

		//Control START clicked
		$('#start').on('click', function () {
			$.ajax({
				url: "/on",
				type: "post",
				data: {
					fan_st: ($("#sliderFanSpeed-switch").prop('checked') ? 1 : 0)
				},
				success: function (response) {
					update_status(false);
				},
				error: function (xhr) {
					lock_ui(true);
				}
			});
		});

		// Finally update status
		update_status(true);

		// Hide preloader overlay when content is loaded
		$('#iot-preloader,.card-preloader').fadeOut();
		$("#wrapper").removeClass("hidden");

		// Check for Main contents scrollbar visibility and set right position for FAB button
		iot.positionFab();
	}, 800);
});

// Apply necessary changes if window resized
$(window).on('resize', function () {
	// Check for Main contents scrollbar visibility and set right position for FAB button
	iot.positionFab();
});

function update_status(rearm) {
	if (status_reading_now) return;
	status_reading_now = true;

	$.ajax({
		url: "/status",
		dataType: 'json',
		type: "get",
		success: function (response) {
			let res = response;

			$('#status-fan-speed').text(res.fan_speed.toFixed(0));
			$('#status-temp-inside').text(res.temp_in.toFixed(0));
			$('#status-temp-heater').text(res.temp_heater.toFixed(0));
			$('#status-humidity-inside').text(res.humid_in.toFixed(0));
			$('#status-temp-outside').text(res.temp_out.toFixed(0));
			$('#status-humidity-outside').text(res.humid_out.toFixed(0));

			$('#status-temp-inside-target').val(res.target_temp_in.toFixed(0)).change().rangeslider('update', true);
			$('#status-temp-heater-max').val(res.max_temp_heater.toFixed(0)).change().rangeslider('update', true);
			$('#sliderFanSpeed').val(res.fan_speed.toFixed(0)).change().rangeslider('update', true);

			(res.in_snsr_finded == 1) ? $('#sensor_inner_status').removeClass('text-danger').addClass('text-success').html('OK') : $('#sensor_inner_status').removeClass('text-success').addClass('text-danger').html('ERROR');
			(res.out_snsr_finded == 1) ? $('#sensor_outer_status').removeClass('text-danger').addClass('text-success').html('OK') : $('#sensor_outer_status').removeClass('text-success').addClass('text-danger').html('ERROR');
			(res.temp_heater > 0) ? $('#sensor_heater_status').removeClass('text-danger').addClass('text-success').html('OK') : $('#sensor_heater_status').removeClass('text-success').addClass('text-danger').html('ERROR');
			(res.fan_st == 1) ? $("#sliderFanSpeed-switch").prop('checked', true).parent().addClass('checked').closest('.card').addClass('active') : $("#sliderFanSpeed-switch").prop('checked', false).parent().removeClass('checked').closest('.card').removeClass('active');

			chart11.updateSeries([
				res.temp_in.toFixed(0)
			]);
			chart12.updateSeries([
				res.humid_in.toFixed(0)
			]);

			// Homed status
			if (res.status == 1) {
				$('#status-box').removeClass('cooling').removeClass('heating').find('.status').removeClass('blink').text('Dry box is on');
				let totalSeconds = Math.floor(res.worktime/1000);
				let hours = Math.floor(totalSeconds / 3600);
				totalSeconds %= 3600;
				let minutes = Math.floor(totalSeconds / 60);
				let seconds = totalSeconds % 60;
				$('#work-time').html(String(hours).padStart(2, "0") + ":" + String(minutes).padStart(2, "0") + ":" + String(seconds).padStart(2, "0"));
				$('#start').addClass('d-none');
				$('#stop').removeClass('d-none');
				$('[data-unit="dry-machine"]').addClass('active').find('.status').html('ON');
				$('#sliderFanSpeed-switch').removeAttr('disabled').parent().removeClass('disabled');
			} else {
				$('#status-box').addClass('cooling').removeClass('heating').find('.status').removeClass('blink').text('Dry box is off');
				$('#work-time').html('--:--:--');
				$('[data-unit="dry-machine"]').removeClass('active').find('.status').html('OFF');
				$('#stop').addClass('d-none');
				$('#start').removeClass('d-none');
				$('#sliderFanSpeed-switch').attr('disabled', '').parent().addClass('disabled');
			}

			update_stats();

			lock_ui(false, res.status);

			status_reading_now = false;
		},
		error: function (xhr) {
			$('#status-box').addClass('heating').removeClass('cooling').find('.status').addClass('blink').text('Dry box is unreachable!');
			lock_ui(true);
			status_reading_now = false;
		}
	});

	if (rearm) {
		rearm_status();
	}
}

function update_stats() {
	$.ajax({
		url: "/stats",
		dataType: 'json',
		type: "get",
		success: function (response) {
			let res = response;
			for (let i = 0; i < res.length; i++) {
				for (let j = 0; j < res[i].length; j++) {
					if (res[i][j] == 0) { res[i][j] = null;}
				}
			}
			chart.updateSeries([
				{
					name: 'Heater temperature',
					type: 'line',
					data: res[0]
				},
				{
					name: 'Temperature inside',
					type: 'line',
					data: res[1]
				}, {
					name: 'Temperature outside',
					type: 'line',
					data: res[2]
				},
				{
					name: 'Humidity inside',
					type: 'line',
					data: res[3]
				},
				{
					name: 'Humidity outside',
					type: 'line',
					data: res[4]
				}
			]);
		},
		error: function (xhr) {
		}
	})
}

function setparams() {
	//Set new params
	$.ajax({
		url: "/setparams",
		type: "post",
		data: {
			trgt_tmpr: $("#status-temp-inside-target").val(),
			max_htr_tmpr: $("#status-temp-heater-max").val(),
			fan_spd: $("#sliderFanSpeed").val(),
			fan_st: ($("#sliderFanSpeed-switch").prop('checked') ? 1 : 0)
		},
		success: function (response) {
		},
		error: function (xhr) {
			lock_ui(true);
		}
	});
}

function lock_ui(lock, status = 0) {
	if (lock) {
		$('#stop,#start').attr('disabled', '');
		$('#sliderFanSpeed-switch').attr('disabled', '').parent().addClass('disabled');
		$('#status-temp-inside-target,#status-temp-heater-max,#sliderFanSpeed').attr('disabled', '').rangeslider('update', true);
		$('#sensor_inner_status,#sensor_outer_status,#sensor_heater_status').removeClass('text-success').html('--');
		$('#status-temp-heater,#status-temp-inside,#status-temp-outside,#status-humidity-inside,#status-humidity-outside,#status-fan-speed').html('--');
		$('#work-time').html('--:--:--');
		chart.updateSeries([
			{
				name: 'Heater temperature',
				type: 'line',
				data: [null, null, null, null, null, null, null, null, null, null, null, null, null, null, null]
			},
			{
				name: 'Temperature inside',
				type: 'line',
				data: [null, null, null, null, null, null, null, null, null, null, null, null, null, null, null]
			}, {
				name: 'Temperature outside',
				type: 'line',
				data: [null, null, null, null, null, null, null, null, null, null, null, null, null, null, null]
			},
			{
				name: 'Humidity inside',
				type: 'line',
				data: [null, null, null, null, null, null, null, null, null, null, null, null, null, null, null]
			},
			{
				name: 'Humidity outside',
				type: 'line',
				data: [null, null, null, null, null, null, null, null, null, null, null, null, null, null, null]
			}
		]);
		chart11.updateSeries([0]);
		chart12.updateSeries([0]);
	} else { //unlock
		$('#stop,#start').removeAttr('disabled');
		if (status == 1) {
			$('#sliderFanSpeed-switch').removeAttr('disabled').parent().removeClass('disabled');
		} else {
			$('#sliderFanSpeed-switch').attr('disabled', '').parent().addClass('disabled');
		}
		$('#status-temp-inside-target,#status-temp-heater-max,#sliderFanSpeed').removeAttr('disabled').rangeslider('update', true);
	}
}

function rearm_status() {
	setTimeout(function () {
		update_status(true);
	}, 4000);
}
