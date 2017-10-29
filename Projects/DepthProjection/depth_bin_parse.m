% % read image
% depth = fopen('C:\Users\Moses\Desktop\d_[00-01-287].bin');
% % binary file 
% depth = fread(depth, [512, 424], 'ushort')';
% % flip the depth
% %depth = depth(:, end:-1:1,:);
% % filter
% depth = depth(:,:,1);
% depth(depth>4095) = 4095;
% d1 = int32(idivide(int32(depth),256));
% d2 = int32(mod(depth,256));
% d3 = depth*0;
% d = uint8(cat(3, d1, d2, d3));
% imwrite(d, 'C:\Users\Moses\Desktop\summer_d.png');

files = dir('D:/Docs/kinect_pics/28353/depth')
for i = 3:length(files)
    filename = files(i).name;

    % read data and do your processing
    % then save with something like:

    infile = ['D:\Docs\kinect_pics\28353\depth\',filename]; 
    % read image
    depth = fopen(infile);
    % binary file 
    depth = fread(depth, [512, 424], 'ushort')';
    % flip the depth
    %depth = depth(:, end:-1:1,:);
    % filter
    depth = depth(:,:,1);
    depth(depth>4095) = 4095;
    depth = double(depth);
    
    str = filename(1:end-4);
    outfile = fullfile('D:/Docs/kinect_pics/28353/smile_d/', [str, '.jpg']);
    imwrite((depth-500)/1500, outfile);
    fclose('all');
end

% % read image
% depth = fopen('C:\Users\Moses\Desktop\d_[00-01-287].bin');
% % binary file 
% depth = fread(depth, [512, 424], 'ushort')';
% % flip the depth
% %depth = depth(:, end:-1:1,:);
% % filter
% depth = depth(:,:,1);
% depth(depth>4095) = 4095;
% depth = double(depth);
% imwrite(depth/4096, 'C:\Users\Moses\Desktop\summer_d.png');