"use strict";
// import { createJWT, isTokenValid, attachCookiesToResponse } from './jwt'
const createTokenUser_1 = require("./createTokenUser");
const checkPermissions_1 = require("./checkPermissions");
const sendVerficationEmail_1 = require("./sendVerficationEmail");
const sendResetPasswordEmail_1 = require("./sendResetPasswordEmail");
const createHash_1 = require("./createHash");
module.exports = {
    createTokenUser: createTokenUser_1.createTokenUser,
    chechPermissions: checkPermissions_1.chechPermissions,
    sendVerificationEmail: sendVerficationEmail_1.sendVerificationEmail,
    sendResetPasswordEmail: sendResetPasswordEmail_1.sendResetPasswordEmail,
    hashString: createHash_1.hashString,
};
