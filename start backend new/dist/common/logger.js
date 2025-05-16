"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.default = void 0;
const winston_1 = __importDefault(require("winston"));
const { combine, timestamp, json } = winston_1.default.format;
const logger = winston_1.default.createLogger({
    level: 'http',
    format: combine(timestamp(), json()),
    transports: [
    // prod transports
    ]
});
exports.default = logger;
// add transports accordingly
// Winston supports (console, file, http, stream)
if (process.env.NODE_ENV !== 'production') {
    logger.add(new winston_1.default.transports.Console({
        format: winston_1.default.format.simple(),
    }));
}
