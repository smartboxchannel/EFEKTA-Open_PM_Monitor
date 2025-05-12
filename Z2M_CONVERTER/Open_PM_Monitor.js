// ############################################################################//
//                                                                             //
//    ... перезагрузить z2m, что бы конвертер применился                       //
//                                                                             //
//#############################################################################//

const {
    numeric,
	pm25,
	reporting,
	light,
	onOff,
	identify,
} = require('zigbee-herdsman-converters/lib/modernExtend');

const pmReporting = {min: 10, max: 600, change: 0.1};
const levelReporting = {min: 10, max: 600, change: 1};


const definition = {
        zigbeeModel: ['Open_PM_Monitor'],
        model: 'Open_PM_Monitor',
        vendor: 'EFEKTA',
        description: 'EFEKTA Open PM Monitor',
        ota:true,
        extend: [
		     identify(),
		     light({
                effect: false,
                powerOnBehavior: false,
				configureReporting: true, 
                levelReportingConfig: levelReporting,

            }),
			 numeric({
                name: "pm1",
                unit: "µg/m³",
                cluster: "pm25Measurement",
                attribute: {ID: 0x0601, type: 0x39},
                description: "Measured PM1.0 (particulate matter) concentration",
                access: "STATE",
                reporting: pmReporting,
                precision: 1,
            }),
			pm25({
                reporting: pmReporting,
                access: "STATE",
                description: "Measured PM2.5 (particulate matter) concentration",
                precision: 1,
            }),
            numeric({
                name: "pm10",
                unit: "µg/m³",
                cluster: "pm25Measurement",
                attribute: {ID: 0x0602, type: 0x39},
                description: "Measured PM10.0 (particulate matter) concentration",
                access: "STATE",
                reporting: pmReporting,
                precision: 1,
            }),
            numeric({
                name: "aqi25",
                unit: "PM2.5 Index",
                cluster: "pm25Measurement",
                attribute: {ID: 0x0604, type: 0x39},
                description: "PM 2.5 INDEX",
                access: "STATE",
                reporting: pmReporting,
            }),
			numeric({
                name: "indicator_corection",
				valueMin: -15,
                valueMax: 15,
                cluster: "pm25Measurement",
                attribute: {ID: 0x0333, type: 0x28},
                description: "Indicator corection",
                access: "STATE_SET",
            }),
        ],
    };

module.exports = definition;