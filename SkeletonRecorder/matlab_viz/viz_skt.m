clear;
clc;

colorList = [[255,0,26]/255; [0,178,51]/255; [0,128,255]/255;...
    [255,128,51]/255; [26,230,230]/255; [77,230,128]/255];

sktFiles = dir('./data/*.txt');
sktFiles(end) = [];

totalFrame = uint16(str2double(sktFiles(end).name(10:14))) + 1;
sktFilesIndex = cell(1, totalFrame);

for sktNum = 1:length(sktFiles)
    frameNum = uint16(str2double(sktFiles(sktNum).name(10:14)));
    sktFilesIndex{frameNum+1} = [sktFilesIndex{frameNum+1}; sktFiles(sktNum).name];
end

for frameNum = 1:totalFrame
    figure(1);
    clf;
    hold on;
    
    % plot skeleton
    for iSkt = 1:size(sktFilesIndex{frameNum}, 1)
        fid = fopen(['./data/', sktFilesIndex{frameNum}(iSkt, :)]);
        sktData = textscan(fid, '%f');
        sktData = sktData{1};
        fclose(fid);

        if isempty(sktData)
            continue;
        end
        
        timestamp = sktData(1);
        skeletonLocation = reshape(sktData(2:126), [5, 25])';
        handState = sktData(127:128);
        skeletonOrientation = reshape(sktData(129:end), [4, 25])';
        
        skeleton3DPosition = skeletonLocation(:, 2:4);
        skeleton3DPosition(:, 1) = -skeleton3DPosition(:, 1);
        plot3dSkeleton(skeleton3DPosition, colorList(iSkt, :));
%         plot3dSkeletonNoHandFoot(skeleton3DPosition, colorList(iSkt, :));
    end
    
    axis off;
    axis equal;
    view(180, -90);
    set(gca,'Units','normalized','Position',[0 0 1 1]);
    cameratoolbar('Show');
    axis([-2, 2, -2, 2, 0.5, 4]);
    
    hold off;
    pause(0.01);
    
    % save data
%     curFrame = getframe(gcf);
%     curFrame = curFrame.cdata;
%     imwrite(curFrame, sprintf('./data/plot_%05d.png', frameNum));
end