"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.resetPassword = exports.forgotPassword = exports.verifyEmail = exports.loginUser = exports.registerUser = exports.logoutUser = exports.userObj = void 0;
const auth_middleware_1 = require("../middleware/auth.middleware");
const StatusCode_1 = require("../enum/StatusCode");
// import { attachCookiesToResponse } from '../middleware/auth.middleware'
// import UserSchema from '../models/UserSchema'
const token_models_1 = __importDefault(require("../models/token.models"));
// import { HttpStatusCodes } from '../enum/StatusCode'
// import * as CustomAPIError from '../errors'
// import { sendResetPasswordEmail, hashString } from '../utils'
const sendResetPasswordEmail_1 = require("../utils/sendResetPasswordEmail");
const createHash_1 = require("../utils/createHash");
const errors_1 = require("../errors");
// import crypto from 'crypto'
const bcrypt_1 = __importDefault(require("bcrypt"));
const validator_1 = __importDefault(require("validator"));
const user_models_1 = require("../models/user.models");
const errors_2 = require("../errors");
const auth_utils_1 = require("../utils/auth.utils");
const dotenv_1 = __importDefault(require("dotenv"));
dotenv_1.default.config();
const registerUser = async (req, res) => {
    const { email, password, username, firstName, lastName, mobile, address, profile, role, } = req.body;
    // Validate input fields
    if (!email || !password || !username) {
        return res.status(400).json({
            status: '400',
            message: 'Please fill all the fields',
        });
    }
    // Validate email format
    if (!validator_1.default.isEmail(email)) {
        return res.status(400).json({
            status: '400',
            message: 'Email is not valid',
        });
    }
    // Validate password strength
    if (!validator_1.default.isStrongPassword(password, {
        minLength: 8,
        minUppercase: 1,
        minNumbers: 1,
        minSymbols: 1,
    })) {
        return res.status(400).json({
            status: '400',
            message: 'Password is not strong enough',
        });
    }
    const usernameSmall = username.toLowerCase();
    // const userAgent = req.get('UserSchema-Agent')
    try {
        // Check if email already exists
        const emailExists = await user_models_1.UserSchema.findOne({ email });
        if (emailExists) {
            return res.status(400).json({
                status: '400',
                message: 'Email already exists',
            });
        }
        // Check if username already exists
        const usernameExists = await user_models_1.UserSchema.findOne({
            username: usernameSmall,
        });
        if (usernameExists) {
            return res.status(400).json({
                status: '400',
                message: 'Username already exists',
            });
        }
        // Generate salt and hash password
        const salt = await bcrypt_1.default.genSalt(10);
        const hashedPassword = await bcrypt_1.default.hash(password, salt);
        // Create new user with optional fields set to null if not provided
        const newUser = new user_models_1.UserSchema({
            email,
            password: hashedPassword,
            username: usernameSmall,
            firstName: firstName || null,
            lastName: lastName || null,
            mobile: mobile || null,
            address: address || null,
            profile: profile || null,
            role,
            verifiedUser: false,
            verifiedDate: null,
        });
        // Save new user
        const newEntry = await newUser.save();
        // reuire for email verification
        // const tempOrigin = req.get('origin')
        // const protocol = req.protocol
        // const host = req.get('host')
        // const forwardedHost = req.get('x-forwarded-host')
        // const forwardedProtocol = req.get('x-forwarded-proto')
        //   await sendVerificationEmail({
        //     name: user.name,
        //     email: user.email,
        //     verificationToken: user.verificationToken,
        //     origin,
        //   })
        // Prepare user data for response
        const userData = {
            username: newEntry.username,
            email: newEntry.email,
            firstName: newEntry.firstName,
            lastName: newEntry.lastName,
            mobile: newEntry.mobile,
            address: newEntry.address,
            profile: newEntry.profile,
            isVerifiedUser: newEntry.verifiedUser,
        };
        // Create tokens
        const accessToken = (0, auth_middleware_1.createAccessToken)(newEntry._id.toString());
        let refreshToken = '';
        // Check for existing token
        const existingToken = await token_models_1.default.findOne({ user: newEntry._id });
        if (existingToken && existingToken.isValid) {
            refreshToken = existingToken.refreshToken;
        }
        else {
            refreshToken = await (0, auth_middleware_1.createRefreshToken)(newEntry._id);
            await token_models_1.default.create({
                user: newEntry._id,
                refreshToken,
                isValid: true,
            });
        }
        // Respond with success
        return res.status(200).json({
            message: 'Registration successful',
            accessToken,
            refreshToken,
            user: userData,
        });
    }
    catch (error) {
        // Log the error for internal debugging
        console.error('Internal Server Error:', error);
        // Respond with appropriate error message
        return res.status(500).json({
            status: '500',
            message: 'User not created due to an internal error. Please try again later.',
        });
    }
};
exports.registerUser = registerUser;
const loginUser = async (req, res) => {
    const { password, username } = req.body;
    // Validate input fields
    if (!password || !username) {
        return res.status(400).json({
            status: '400',
            message: 'Please fill all the fields',
        });
    }
    try {
        const usernameSmall = username.toLowerCase();
        // const userAgent = req.get('UserSchema-Agent')
        const userObj = await user_models_1.UserSchema.findOne({ username: usernameSmall });
        if (!userObj) {
            return res.status(404).json({
                message: 'User not found',
                status: '404 Not Found',
            });
        }
        const userPassword = userObj.password || '';
        const validPassword = await bcrypt_1.default.compare(req.body.password, userPassword);
        if (!validPassword) {
            return res.status(400).json({
                message: 'Invalid password',
                status: '400',
            });
        }
        const userData = {
            _id: userObj._id,
            username: userObj.username,
            email: userObj.email,
            firstName: userObj.firstName,
            lastName: userObj.lastName,
            mobile: userObj.mobile,
            address: userObj.address,
            profile: userObj.profile,
        };
        const accessToken = (0, auth_middleware_1.createAccessToken)(`${userObj._id}`);
        let refreshToken = '';
        // check for existing token
        const existingToken = await token_models_1.default.findOne({ user: userObj._id });
        if (existingToken) {
            const { isValid } = existingToken;
            if (!isValid) {
                return res.status(200).json({
                    message: 'Anuthorized Access',
                    status: '200',
                });
            }
            refreshToken = existingToken.refreshToken;
            // attachCookiesToResponse({ res, accessToken, refreshToken })
        }
        else {
            refreshToken = await (0, auth_middleware_1.createRefreshToken)(userObj._id);
            // attachCookiesToResponse({ res, accessToken, refreshToken })
        }
        return res.status(200).json({
            message: 'Login successful',
            accessToken,
            refreshToken,
            user: userData,
        });
    }
    catch (error) {
        console.error(error);
        return res.status(500).json({
            status: '500',
            message: 'Internal Server Error. Please check later.',
        });
    }
};
exports.loginUser = loginUser;
const logoutUser = async (req, res) => {
    const { userID } = req.body;
    if (!userID) {
        res.status(400).json({
            status: '400',
            message: 'user id is missing',
        });
    }
    await token_models_1.default.findOneAndDelete({ user: userID });
    res.cookie('accessToken', 'logout', {
        httpOnly: true,
        expires: new Date(Date.now()),
    });
    res.status(StatusCode_1.HttpStatusCodes.OK).json({ msg: 'user logged out!' });
};
exports.logoutUser = logoutUser;
const verifyEmail = async (req, res) => {
    const { verificationToken, username } = req.body;
    const user = await user_models_1.UserSchema.findOne({ username });
    if (!user) {
        throw new errors_2.CustomAPIError.UnauthorizedError('Verification Failed');
    }
    if (user.verificationToken !== verificationToken) {
        throw new errors_2.CustomAPIError.UnauthorizedError('Verification Failed');
    }
    (user.verifiedUser = true), (user.verificationToken = '');
    user.verifiedDate = new Date(); // Use a date string for consistency
    await user.save();
    res.status(200).json({ msg: 'Email Verified' });
};
exports.verifyEmail = verifyEmail;
const userObj = async (req, res) => {
    const { username, password } = req.body;
    if (!username || !password) {
        throw new errors_1.BadRequestError('Please provide email and password');
    }
    const userObj = await user_models_1.UserSchema.findOne({ username });
    if (!userObj) {
        throw new errors_2.CustomAPIError.UnauthorizedError('Invalid Credentials');
    }
    const isPasswordCorrect = await (0, auth_utils_1.comparePassword)(password, userObj.password);
    if (!isPasswordCorrect) {
        throw new errors_2.CustomAPIError.UnauthorizedError('Invalid Credentials');
    }
    if (!userObj.verifiedUser) {
        throw new errors_2.CustomAPIError.UnauthorizedError('Please verify your email');
    }
    const userData = {
        _id: userObj._id,
        username: userObj.username,
        email: userObj.email,
        firstName: userObj.firstName,
        lastName: userObj.lastName,
        mobile: userObj.mobile,
        address: userObj.address,
        profile: userObj.profile,
    };
    return res.status(200).json({
        user: userData,
    });
};
exports.userObj = userObj;
const forgotPassword = async (req, res) => {
    const { username } = req.body;
    if (!username) {
        throw new errors_1.BadRequestError('Please provide valid email');
    }
    const user = await user_models_1.UserSchema.findOne({ username });
    if (user) {
        const passwordToken = (0, auth_middleware_1.createAccessToken)(`${user._id}`);
        // send email
        const origin = 'http://localhost:3000';
        await (0, sendResetPasswordEmail_1.sendResetPasswordEmail)({
            name: `${user.firstName} ${user.lastName}`,
            email: user.email,
            token: passwordToken,
            origin,
        });
        const tenMinutes = 1000 * 60 * 10;
        const passwordTokenExpirationDate = new Date(Date.now() + tenMinutes);
        user.verificationToken = (0, createHash_1.hashString)(passwordToken);
        user.tokenExpirationDate = passwordTokenExpirationDate;
        await user.save();
    }
    res
        .status(StatusCode_1.HttpStatusCodes.OK)
        .json({ msg: 'Please check your email for reset password link' });
};
exports.forgotPassword = forgotPassword;
const resetPassword = async (req, res) => {
    const { token, username, password } = req.body;
    if (!token || !username || !password) {
        throw new errors_1.BadRequestError('Please provide all values');
    }
    const user = await user_models_1.UserSchema.findOne({ username });
    if (user) {
        if (user.verificationToken === (0, createHash_1.hashString)(token)) {
            user.password = password;
            // user.verificationToken = null
            // user.tokenExpirationDate = null
            await user.save();
        }
    }
    res.send('reset password');
};
exports.resetPassword = resetPassword;
