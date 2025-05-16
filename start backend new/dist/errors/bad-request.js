"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.BadRequestError = void 0;
const StatusCode_1 = require("../enum/StatusCode");
const custom_api_1 = require("./custom-api");
class BadRequestError extends custom_api_1.CustomAPIError {
    statusCode;
    constructor(message) {
        super(message);
        this.statusCode = StatusCode_1.HttpStatusCodes.BAD_REQUEST;
    }
}
exports.BadRequestError = BadRequestError;
