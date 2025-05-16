"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.httpLogger = void 0;
const morgan_1 = __importDefault(require("morgan"));
const logger_1 = __importDefault(require("../common/logger"));
const httpLogger = (0, morgan_1.default)('tiny', {
    stream: {
        write: (message) => logger_1.default.http(message.trim()),
    },
});
exports.httpLogger = httpLogger;
