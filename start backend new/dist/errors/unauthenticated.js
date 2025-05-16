"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.UnauthenticatedError = void 0;
const StatusCode_1 = require("../enum/StatusCode");
const custom_api_1 = require("./custom-api");
class UnauthenticatedError extends custom_api_1.CustomAPIError {
    statusCode;
    constructor(message) {
        super(message);
        this.statusCode = StatusCode_1.HttpStatusCodes.UNAUTHORIZED;
    }
}
exports.UnauthenticatedError = UnauthenticatedError;
